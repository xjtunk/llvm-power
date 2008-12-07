//===- LLVMCConfigurationEmitter.cpp - Generate LLVMC config --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open
// Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This tablegen backend is responsible for emitting LLVMC configuration code.
//
//===----------------------------------------------------------------------===//

#include "LLVMCConfigurationEmitter.h"
#include "Record.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Streams.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <string>

using namespace llvm;

namespace {

//===----------------------------------------------------------------------===//
/// Typedefs

typedef std::vector<Record*> RecordVector;
typedef std::vector<std::string> StrVector;

//===----------------------------------------------------------------------===//
/// Constants

// Indentation strings.
const char * Indent1 = "    ";
const char * Indent2 = "        ";
const char * Indent3 = "            ";
const char * Indent4 = "                ";

// Default help string.
const char * DefaultHelpString = "NO HELP MESSAGE PROVIDED";

// Name for the "sink" option.
const char * SinkOptionName = "AutoGeneratedSinkOption";

//===----------------------------------------------------------------------===//
/// Helper functions

std::string InitPtrToString(Init* ptr) {
  StringInit& val = dynamic_cast<StringInit&>(*ptr);
  return val.getValue();
}

int InitPtrToInt(Init* ptr) {
  IntInit& val = dynamic_cast<IntInit&>(*ptr);
  return val.getValue();
}

const DagInit& InitPtrToDagInitRef(Init* ptr) {
  DagInit& val = dynamic_cast<DagInit&>(*ptr);
  return val;
}


// checkNumberOfArguments - Ensure that the number of args in d is
// less than or equal to min_arguments, otherwise throw an exception .
void checkNumberOfArguments (const DagInit* d, unsigned min_arguments) {
  if (d->getNumArgs() < min_arguments)
    throw "Property " + d->getOperator()->getAsString()
      + " has too few arguments!";
}


//===----------------------------------------------------------------------===//
/// Back-end specific code

// A command-line option can have one of the following types:
//
// Switch - a simple switch w/o arguments, e.g. -O2
//
// Parameter - an option that takes one(and only one) argument, e.g. -o file,
// --output=file
//
// ParameterList - same as Parameter, but more than one occurence
// of the option is allowed, e.g. -lm -lpthread
//
// Prefix - argument is everything after the prefix,
// e.g. -Wa,-foo,-bar, -DNAME=VALUE
//
// PrefixList - same as Prefix, but more than one option occurence is
// allowed

namespace OptionType {
  enum OptionType { Switch, Parameter, ParameterList, Prefix, PrefixList};
}

bool IsListOptionType (OptionType::OptionType t) {
  return (t == OptionType::ParameterList || t == OptionType::PrefixList);
}

// Code duplication here is necessary because one option can affect
// several tools and those tools may have different actions associated
// with this option. GlobalOptionDescriptions are used to generate
// the option registration code, while ToolOptionDescriptions are used
// to generate tool-specific code.

/// OptionDescription - Base class for option descriptions.
struct OptionDescription {
  OptionType::OptionType Type;
  std::string Name;

  OptionDescription(OptionType::OptionType t = OptionType::Switch,
                    const std::string& n = "")
  : Type(t), Name(n)
  {}

  const char* GenTypeDeclaration() const {
    switch (Type) {
    case OptionType::PrefixList:
    case OptionType::ParameterList:
      return "cl::list<std::string>";
    case OptionType::Switch:
      return "cl::opt<bool>";
    case OptionType::Parameter:
    case OptionType::Prefix:
    default:
      return "cl::opt<std::string>";
    }
  }

  std::string GenVariableName() const {
    switch (Type) {
    case OptionType::Switch:
     return "AutoGeneratedSwitch" + Name;
   case OptionType::Prefix:
     return "AutoGeneratedPrefix" + Name;
   case OptionType::PrefixList:
     return "AutoGeneratedPrefixList" + Name;
   case OptionType::Parameter:
     return "AutoGeneratedParameter" + Name;
   case OptionType::ParameterList:
   default:
     return "AutoGeneratedParameterList" + Name;
   }
  }

};

// Global option description.

namespace GlobalOptionDescriptionFlags {
  enum GlobalOptionDescriptionFlags { Required = 0x1 };
}

struct GlobalOptionDescription : public OptionDescription {
  std::string Help;
  unsigned Flags;

  // We need to provide a default constructor because
  // StringMap can only store DefaultConstructible objects.
  GlobalOptionDescription() : OptionDescription(), Flags(0)
  {}

  GlobalOptionDescription (OptionType::OptionType t, const std::string& n)
    : OptionDescription(t, n), Help(DefaultHelpString), Flags(0)
  {}

  bool isRequired() const {
    return Flags & GlobalOptionDescriptionFlags::Required;
  }
  void setRequired() {
    Flags |= GlobalOptionDescriptionFlags::Required;
  }

  /// Merge - Merge two option descriptions.
  void Merge (const GlobalOptionDescription& other)
  {
    if (other.Type != Type)
      throw "Conflicting definitions for the option " + Name + "!";

    if (Help == DefaultHelpString)
      Help = other.Help;
    else if (other.Help != DefaultHelpString) {
      llvm::cerr << "Warning: more than one help string defined for option "
        + Name + "\n";
    }

    Flags |= other.Flags;
  }
};

/// GlobalOptionDescriptions - A GlobalOptionDescription array
/// together with some flags affecting generation of option
/// declarations.
struct GlobalOptionDescriptions {
  typedef StringMap<GlobalOptionDescription> container_type;
  typedef container_type::const_iterator const_iterator;

  /// Descriptions - A list of GlobalOptionDescriptions.
  container_type Descriptions;
  /// HasSink - Should the emitter generate a "cl::sink" option?
  bool HasSink;

  const GlobalOptionDescription& FindOption(const std::string& OptName) const {
    const_iterator I = Descriptions.find(OptName);
    if (I != Descriptions.end())
      return I->second;
    else
      throw OptName + ": no such option!";
  }

  // Support for STL-style iteration
  const_iterator begin() const { return Descriptions.begin(); }
  const_iterator end() const { return Descriptions.end(); }
};


// Tool-local option description.

// Properties without arguments are implemented as flags.
namespace ToolOptionDescriptionFlags {
  enum ToolOptionDescriptionFlags { StopCompilation = 0x1,
                                    Forward = 0x2, UnpackValues = 0x4};
}
namespace OptionPropertyType {
  enum OptionPropertyType { AppendCmd };
}

typedef std::pair<OptionPropertyType::OptionPropertyType, std::string>
OptionProperty;
typedef SmallVector<OptionProperty, 4> OptionPropertyList;

struct ToolOptionDescription : public OptionDescription {
  unsigned Flags;
  OptionPropertyList Props;

  // StringMap can only store DefaultConstructible objects
  ToolOptionDescription() : OptionDescription(), Flags(0) {}

  ToolOptionDescription (OptionType::OptionType t, const std::string& n)
    : OptionDescription(t, n)
  {}

  // Various boolean properties
  bool isStopCompilation() const {
    return Flags & ToolOptionDescriptionFlags::StopCompilation;
  }
  void setStopCompilation() {
    Flags |= ToolOptionDescriptionFlags::StopCompilation;
  }

  bool isForward() const {
    return Flags & ToolOptionDescriptionFlags::Forward;
  }
  void setForward() {
    Flags |= ToolOptionDescriptionFlags::Forward;
  }

  bool isUnpackValues() const {
    return Flags & ToolOptionDescriptionFlags::UnpackValues;
  }
  void setUnpackValues() {
    Flags |= ToolOptionDescriptionFlags::UnpackValues;
  }

  void AddProperty (OptionPropertyType::OptionPropertyType t,
                    const std::string& val)
  {
    Props.push_back(std::make_pair(t, val));
  }
};

typedef StringMap<ToolOptionDescription> ToolOptionDescriptions;

// Tool information record

namespace ToolFlags {
  enum ToolFlags { Join = 0x1, Sink = 0x2 };
}

struct ToolProperties : public RefCountedBase<ToolProperties> {
  std::string Name;
  StrVector CmdLine;
  std::string InLanguage;
  std::string OutLanguage;
  std::string OutputSuffix;
  unsigned Flags;
  ToolOptionDescriptions OptDescs;

  // Various boolean properties
  void setSink()      { Flags |= ToolFlags::Sink; }
  bool isSink() const { return Flags & ToolFlags::Sink; }
  void setJoin()      { Flags |= ToolFlags::Join; }
  bool isJoin() const { return Flags & ToolFlags::Join; }

  // Default ctor here is needed because StringMap can only store
  // DefaultConstructible objects
  ToolProperties() : Flags(0) {}
  ToolProperties (const std::string& n) : Name(n), Flags(0) {}
};


/// ToolPropertiesList - A list of Tool information records
/// IntrusiveRefCntPtrs are used here because StringMap has no copy
/// constructor (and we want to avoid copying ToolProperties anyway).
typedef std::vector<IntrusiveRefCntPtr<ToolProperties> > ToolPropertiesList;


/// CollectProperties - Function object for iterating over a list of
/// tool property records.
class CollectProperties {
private:

  /// Implementation details

  /// PropertyHandler - a function that extracts information
  /// about a given tool property from its DAG representation
  typedef void (CollectProperties::*PropertyHandler)(const DagInit*);

  /// PropertyHandlerMap - A map from property names to property
  /// handlers.
  typedef StringMap<PropertyHandler> PropertyHandlerMap;

  /// OptionPropertyHandler - a function that extracts information
  /// about a given option property from its DAG representation.
  typedef void (CollectProperties::* OptionPropertyHandler)
  (const DagInit*, GlobalOptionDescription &);

  /// OptionPropertyHandlerMap - A map from option property names to
  /// option property handlers
  typedef StringMap<OptionPropertyHandler> OptionPropertyHandlerMap;

  // Static maps from strings to CollectProperties methods("handlers")
  static PropertyHandlerMap propertyHandlers_;
  static OptionPropertyHandlerMap optionPropertyHandlers_;
  static bool staticMembersInitialized_;


  /// This is where the information is stored

  /// toolProps_ -  Properties of the current Tool.
  ToolProperties& toolProps_;
  /// optDescs_ - OptionDescriptions table (used to register options
  /// globally).
  GlobalOptionDescriptions& optDescs_;

public:

  explicit CollectProperties (ToolProperties& p, GlobalOptionDescriptions& d)
    : toolProps_(p), optDescs_(d)
  {
    if (!staticMembersInitialized_) {
      // Init tool property handlers
      propertyHandlers_["cmd_line"] = &CollectProperties::onCmdLine;
      propertyHandlers_["in_language"] = &CollectProperties::onInLanguage;
      propertyHandlers_["join"] = &CollectProperties::onJoin;
      propertyHandlers_["out_language"] = &CollectProperties::onOutLanguage;
      propertyHandlers_["output_suffix"] = &CollectProperties::onOutputSuffix;
      propertyHandlers_["parameter_option"]
        = &CollectProperties::onParameter;
      propertyHandlers_["parameter_list_option"] =
        &CollectProperties::onParameterList;
      propertyHandlers_["prefix_option"] = &CollectProperties::onPrefix;
      propertyHandlers_["prefix_list_option"] =
        &CollectProperties::onPrefixList;
      propertyHandlers_["sink"] = &CollectProperties::onSink;
      propertyHandlers_["switch_option"] = &CollectProperties::onSwitch;

      // Init option property handlers
      optionPropertyHandlers_["append_cmd"] = &CollectProperties::onAppendCmd;
      optionPropertyHandlers_["forward"] = &CollectProperties::onForward;
      optionPropertyHandlers_["help"] = &CollectProperties::onHelp;
      optionPropertyHandlers_["required"] = &CollectProperties::onRequired;
      optionPropertyHandlers_["stop_compilation"] =
        &CollectProperties::onStopCompilation;
      optionPropertyHandlers_["unpack_values"] =
        &CollectProperties::onUnpackValues;

      staticMembersInitialized_ = true;
    }
  }

  /// operator() - Gets called for every tool property; Just forwards
  /// to the corresponding property handler.
  void operator() (Init* i) {
    const DagInit& d = InitPtrToDagInitRef(i);
    const std::string& property_name = d.getOperator()->getAsString();
    PropertyHandlerMap::iterator method
      = propertyHandlers_.find(property_name);

    if (method != propertyHandlers_.end()) {
      PropertyHandler h = method->second;
      (this->*h)(&d);
    }
    else {
      throw "Unknown tool property: " + property_name + "!";
    }
  }

private:

  /// Property handlers --
  /// Functions that extract information about tool properties from
  /// DAG representation.

  void onCmdLine (const DagInit* d) {
    checkNumberOfArguments(d, 1);
    SplitString(InitPtrToString(d->getArg(0)), toolProps_.CmdLine);
    if (toolProps_.CmdLine.empty())
      throw "Tool " + toolProps_.Name + " has empty command line!";
  }

  void onInLanguage (const DagInit* d) {
    checkNumberOfArguments(d, 1);
    toolProps_.InLanguage = InitPtrToString(d->getArg(0));
  }

  void onJoin (const DagInit* d) {
    checkNumberOfArguments(d, 0);
    toolProps_.setJoin();
  }

  void onOutLanguage (const DagInit* d) {
    checkNumberOfArguments(d, 1);
    toolProps_.OutLanguage = InitPtrToString(d->getArg(0));
  }

  void onOutputSuffix (const DagInit* d) {
    checkNumberOfArguments(d, 1);
    toolProps_.OutputSuffix = InitPtrToString(d->getArg(0));
  }

  void onSink (const DagInit* d) {
    checkNumberOfArguments(d, 0);
    optDescs_.HasSink = true;
    toolProps_.setSink();
  }

  void onSwitch (const DagInit* d) {
    addOption(d, OptionType::Switch);
  }

  void onParameter (const DagInit* d) {
    addOption(d, OptionType::Parameter);
  }

  void onParameterList (const DagInit* d) {
    addOption(d, OptionType::ParameterList);
  }

  void onPrefix (const DagInit* d) {
    addOption(d, OptionType::Prefix);
  }

  void onPrefixList (const DagInit* d) {
    addOption(d, OptionType::PrefixList);
  }

  /// Option property handlers --
  /// Methods that handle properties that are common for all types of
  /// options (like append_cmd, stop_compilation)

  void onAppendCmd (const DagInit* d, GlobalOptionDescription& o) {
    checkNumberOfArguments(d, 1);
    std::string const& cmd = InitPtrToString(d->getArg(0));

    toolProps_.OptDescs[o.Name].AddProperty(OptionPropertyType::AppendCmd, cmd);
  }

  void onForward (const DagInit* d, GlobalOptionDescription& o) {
    checkNumberOfArguments(d, 0);
    toolProps_.OptDescs[o.Name].setForward();
  }

  void onHelp (const DagInit* d, GlobalOptionDescription& o) {
    checkNumberOfArguments(d, 1);
    const std::string& help_message = InitPtrToString(d->getArg(0));

    o.Help = help_message;
  }

  void onRequired (const DagInit* d, GlobalOptionDescription& o) {
    checkNumberOfArguments(d, 0);
    o.setRequired();
  }

  void onStopCompilation (const DagInit* d, GlobalOptionDescription& o) {
    checkNumberOfArguments(d, 0);
    if (o.Type != OptionType::Switch)
      throw std::string("Only options of type Switch can stop compilation!");
    toolProps_.OptDescs[o.Name].setStopCompilation();
  }

  void onUnpackValues (const DagInit* d, GlobalOptionDescription& o) {
    checkNumberOfArguments(d, 0);
    toolProps_.OptDescs[o.Name].setUnpackValues();
  }

  /// Helper functions

  // Add an option of type t
  void addOption (const DagInit* d, OptionType::OptionType t) {
    checkNumberOfArguments(d, 2);
    const std::string& name = InitPtrToString(d->getArg(0));

    GlobalOptionDescription o(t, name);
    toolProps_.OptDescs[name].Type = t;
    toolProps_.OptDescs[name].Name = name;
    processOptionProperties(d, o);
    insertDescription(o);
  }

  // Insert new GlobalOptionDescription into GlobalOptionDescriptions list
  void insertDescription (const GlobalOptionDescription& o)
  {
    if (optDescs_.Descriptions.count(o.Name)) {
      GlobalOptionDescription& D = optDescs_.Descriptions[o.Name];
      D.Merge(o);
    }
    else {
      optDescs_.Descriptions[o.Name] = o;
    }
  }

  /// processOptionProperties - Go through the list of option
  /// properties and call a corresponding handler for each.
  ///
  /// Parameters:
  /// name - option name
  /// d - option property list
  void processOptionProperties (const DagInit* d, GlobalOptionDescription& o) {
    // First argument is option name
    checkNumberOfArguments(d, 2);

    for (unsigned B = 1, E = d->getNumArgs(); B!=E; ++B) {
      const DagInit& option_property
        = InitPtrToDagInitRef(d->getArg(B));
      const std::string& option_property_name
        = option_property.getOperator()->getAsString();
      OptionPropertyHandlerMap::iterator method
        = optionPropertyHandlers_.find(option_property_name);

      if (method != optionPropertyHandlers_.end()) {
        OptionPropertyHandler h = method->second;
        (this->*h)(&option_property, o);
      }
      else {
        throw "Unknown option property: " + option_property_name + "!";
      }
    }
  }
};

// Static members of CollectProperties
CollectProperties::PropertyHandlerMap
CollectProperties::propertyHandlers_;

CollectProperties::OptionPropertyHandlerMap
CollectProperties::optionPropertyHandlers_;

bool CollectProperties::staticMembersInitialized_ = false;


/// CollectToolProperties - Gather information from the parsed
/// TableGen data (basically a wrapper for the CollectProperties
/// function object).
void CollectToolProperties (RecordVector::const_iterator B,
                            RecordVector::const_iterator E,
                            ToolPropertiesList& TPList,
                            GlobalOptionDescriptions& OptDescs)
{
  // Iterate over a properties list of every Tool definition
  for (;B!=E;++B) {
    RecordVector::value_type T = *B;
    ListInit* PropList = T->getValueAsListInit("properties");

    IntrusiveRefCntPtr<ToolProperties>
      ToolProps(new ToolProperties(T->getName()));

    std::for_each(PropList->begin(), PropList->end(),
                  CollectProperties(*ToolProps, OptDescs));
    TPList.push_back(ToolProps);
  }
}

/// EmitForwardOptionPropertyHandlingCode - Helper function used to
/// implement EmitOptionPropertyHandlingCode(). Emits code for
/// handling the (forward) option property.
void EmitForwardOptionPropertyHandlingCode (const ToolOptionDescription& D,
                                            std::ostream& O) {
  switch (D.Type) {
  case OptionType::Switch:
    O << Indent3 << "vec.push_back(\"-" << D.Name << "\");\n";
    break;
  case OptionType::Parameter:
    O << Indent3 << "vec.push_back(\"-" << D.Name << "\");\n";
    O << Indent3 << "vec.push_back(" << D.GenVariableName() << ");\n";
    break;
  case OptionType::Prefix:
    O << Indent3 << "vec.push_back(\"-" << D.Name << "\" + "
      << D.GenVariableName() << ");\n";
    break;
  case OptionType::PrefixList:
    O << Indent3 << "for (" << D.GenTypeDeclaration()
      << "::iterator B = " << D.GenVariableName() << ".begin(),\n"
      << Indent3 << "E = " << D.GenVariableName() << ".end(); B != E; ++B)\n"
      << Indent4 << "vec.push_back(\"-" << D.Name << "\" + "
      << "*B);\n";
    break;
  case OptionType::ParameterList:
    O << Indent3 << "for (" << D.GenTypeDeclaration()
      << "::iterator B = " << D.GenVariableName() << ".begin(),\n"
      << Indent3 << "E = " << D.GenVariableName()
      << ".end() ; B != E; ++B) {\n"
      << Indent4 << "vec.push_back(\"-" << D.Name << "\");\n"
      << Indent4 << "vec.push_back(*B);\n"
      << Indent3 << "}\n";
    break;
  }
}

/// EmitOptionPropertyHandlingCode - Helper function used by
/// EmitGenerateActionMethod(). Emits code that handles option
/// properties.
void EmitOptionPropertyHandlingCode (const ToolProperties& P,
                                     const ToolOptionDescription& D,
                                     std::ostream& O)
{
  // Start of the if-clause.
  O << Indent2 << "if (";
  if (D.Type == OptionType::Switch)
    O << D.GenVariableName();
  else
    O << '!' << D.GenVariableName() << ".empty()";

  O <<") {\n";

  // Handle option properties that take an argument.
  for (OptionPropertyList::const_iterator B = D.Props.begin(),
        E = D.Props.end(); B!=E; ++B) {
    const OptionProperty& val = *B;

    switch (val.first) {
      // (append_cmd cmd) property
    case OptionPropertyType::AppendCmd:
      O << Indent3 << "vec.push_back(\"" << val.second << "\");\n";
      break;
      // Other properties with argument
    default:
      break;
    }
  }

  // Handle flags

  // (forward) property
  if (D.isForward())
    EmitForwardOptionPropertyHandlingCode(D, O);

  // (unpack_values) property
  if (D.isUnpackValues()) {
    if (IsListOptionType(D.Type)) {
      O << Indent3 << "for (" << D.GenTypeDeclaration()
        << "::iterator B = " << D.GenVariableName() << ".begin(),\n"
        << Indent3 << "E = " << D.GenVariableName()
        << ".end(); B != E; ++B)\n"
        << Indent4 << "llvm::SplitString(*B, vec, \",\");\n";
    }
    else if (D.Type == OptionType::Prefix || D.Type == OptionType::Parameter){
      O << Indent3 << "llvm::SplitString("
        << D.GenVariableName() << ", vec, \",\");\n";
    }
    else {
      // TOFIX: move this to the type-checking phase
      throw std::string("Switches can't have unpack_values property!");
    }
  }

  // End of the if-clause.
  O << Indent2 << "}\n";
}

// EmitGenerateActionMethod - Emit one of two versions of the
// Tool::GenerateAction() method.
void EmitGenerateActionMethod (const ToolProperties& P, bool V, std::ostream& O)
{
  if (V)
    O << Indent1 << "Action GenerateAction(const PathVector& inFiles,\n";
  else
    O << Indent1 << "Action GenerateAction(const sys::Path& inFile,\n";

  O << Indent2 << "const sys::Path& outFile) const\n"
    << Indent1 << "{\n"
    << Indent2 << "std::vector<std::string> vec;\n";

  // Parse CmdLine tool property
  if(P.CmdLine.empty())
    throw "Tool " + P.Name + " has empty command line!";

  StrVector::const_iterator I = P.CmdLine.begin();
  ++I;
  for (StrVector::const_iterator E = P.CmdLine.end(); I != E; ++I) {
    const std::string& cmd = *I;
    O << Indent2;
    if (cmd == "$INFILE") {
      if (V)
        O << "for (PathVector::const_iterator B = inFiles.begin()"
          << ", E = inFiles.end();\n"
          << Indent2 << "B != E; ++B)\n"
          << Indent3 << "vec.push_back(B->toString());\n";
      else
        O << "vec.push_back(inFile.toString());\n";
    }
    else if (cmd == "$OUTFILE") {
      O << "vec.push_back(outFile.toString());\n";
    }
    else {
      O << "vec.push_back(\"" << cmd << "\");\n";
    }
  }

  // For every understood option, emit handling code.
  for (ToolOptionDescriptions::const_iterator B = P.OptDescs.begin(),
        E = P.OptDescs.end(); B != E; ++B) {
    const ToolOptionDescription& val = B->second;
    EmitOptionPropertyHandlingCode(P, val, O);
  }

  // Handle the Sink property.
  if (P.isSink()) {
    O << Indent2 << "if (!" << SinkOptionName << ".empty()) {\n"
      << Indent3 << "vec.insert(vec.end(), "
      << SinkOptionName << ".begin(), " << SinkOptionName << ".end());\n"
      << Indent2 << "}\n";
  }

  O << Indent2 << "return Action(\"" << P.CmdLine.at(0) << "\", vec);\n"
    << Indent1 << "}\n\n";
}

/// EmitGenerateActionMethods - Emit two GenerateAction() methods for
/// a given Tool class.
void EmitGenerateActionMethods (const ToolProperties& P, std::ostream& O) {

  if (!P.isJoin())
    O << Indent1 << "Action GenerateAction(const PathVector& inFiles,\n"
      << Indent2 << "const llvm::sys::Path& outFile) const\n"
      << Indent1 << "{\n"
      << Indent2 << "throw std::runtime_error(\"" << P.Name
      << " is not a Join tool!\");\n"
      << Indent1 << "}\n\n";
  else
    EmitGenerateActionMethod(P, true, O);

  EmitGenerateActionMethod(P, false, O);
}

/// EmitIsLastMethod - Emit the IsLast() method for a given Tool
/// class.
void EmitIsLastMethod (const ToolProperties& P, std::ostream& O) {
  O << Indent1 << "bool IsLast() const {\n"
    << Indent2 << "bool last = false;\n";

  for (ToolOptionDescriptions::const_iterator B = P.OptDescs.begin(),
        E = P.OptDescs.end(); B != E; ++B) {
    const ToolOptionDescription& val = B->second;

    if (val.isStopCompilation())
      O << Indent2
        << "if (" << val.GenVariableName()
        << ")\n" << Indent3 << "last = true;\n";
  }

  O << Indent2 << "return last;\n"
    << Indent1 <<  "}\n\n";
}

/// EmitInOutLanguageMethods - Emit the [Input,Output]Language()
/// methods for a given Tool class.
void EmitInOutLanguageMethods (const ToolProperties& P, std::ostream& O) {
  O << Indent1 << "const char* InputLanguage() const {\n"
    << Indent2 << "return \"" << P.InLanguage << "\";\n"
    << Indent1 << "}\n\n";

  O << Indent1 << "const char* OutputLanguage() const {\n"
    << Indent2 << "return \"" << P.OutLanguage << "\";\n"
    << Indent1 << "}\n\n";
}

/// EmitOutputSuffixMethod - Emit the OutputSuffix() method for a
/// given Tool class.
void EmitOutputSuffixMethod (const ToolProperties& P, std::ostream& O) {
  O << Indent1 << "const char* OutputSuffix() const {\n"
    << Indent2 << "return \"" << P.OutputSuffix << "\";\n"
    << Indent1 << "}\n\n";
}

/// EmitNameMethod - Emit the Name() method for a given Tool class.
void EmitNameMethod (const ToolProperties& P, std::ostream& O) {
  O << Indent1 << "const char* Name() const {\n"
    << Indent2 << "return \"" << P.Name << "\";\n"
    << Indent1 << "}\n\n";
}

/// EmitIsJoinMethod - Emit the IsJoin() method for a given Tool
/// class.
void EmitIsJoinMethod (const ToolProperties& P, std::ostream& O) {
  O << Indent1 << "bool IsJoin() const {\n";
  if (P.isJoin())
    O << Indent2 << "return true;\n";
  else
    O << Indent2 << "return false;\n";
  O << Indent1 << "}\n\n";
}

/// EmitToolClassDefinition - Emit a Tool class definition.
void EmitToolClassDefinition (const ToolProperties& P, std::ostream& O) {

  if(P.Name == "root")
    return;

  // Header
  O << "class " << P.Name << " : public ";
  if (P.isJoin())
    O << "JoinTool";
  else
    O << "Tool";
  O << " {\npublic:\n";

  EmitNameMethod(P, O);
  EmitInOutLanguageMethods(P, O);
  EmitOutputSuffixMethod(P, O);
  EmitIsJoinMethod(P, O);
  EmitGenerateActionMethods(P, O);
  EmitIsLastMethod(P, O);

  // Close class definition
  O << "};\n\n";
}

/// EmitOptionDescriptions - Iterate over a list of option
/// descriptions and emit registration code.
void EmitOptionDescriptions (const GlobalOptionDescriptions& descs,
                             std::ostream& O)
{
  // Emit static cl::Option variables
  for (GlobalOptionDescriptions::const_iterator B = descs.begin(),
         E = descs.end(); B!=E; ++B) {
    const GlobalOptionDescription& val = B->second;

    O << val.GenTypeDeclaration() << ' '
      << val.GenVariableName()
      << "(\"" << val.Name << '\"';

    if (val.Type == OptionType::Prefix || val.Type == OptionType::PrefixList)
      O << ", cl::Prefix";

    if (val.isRequired()) {
      switch (val.Type) {
      case OptionType::PrefixList:
      case OptionType::ParameterList:
        O << ", cl::OneOrMore";
        break;
      default:
        O << ", cl::Required";
      }
    }

    O << ", cl::desc(\"" << val.Help << "\"));\n";
  }

  if (descs.HasSink)
    O << "cl::list<std::string> " << SinkOptionName << "(cl::Sink);\n";

  O << '\n';
}

/// EmitPopulateLanguageMap - Emit the PopulateLanguageMap() function.
void EmitPopulateLanguageMap (const RecordKeeper& Records, std::ostream& O)
{
  // Get the relevant field out of RecordKeeper
  Record* LangMapRecord = Records.getDef("LanguageMap");
  if (!LangMapRecord)
    throw std::string("Language map definition not found!");

  ListInit* LangsToSuffixesList = LangMapRecord->getValueAsListInit("map");
  if (!LangsToSuffixesList)
    throw std::string("Error in the language map definition!");

  // Generate code
  O << "void llvmc::PopulateLanguageMap(LanguageMap& language_map) {\n";

  for (unsigned i = 0; i < LangsToSuffixesList->size(); ++i) {
    Record* LangToSuffixes = LangsToSuffixesList->getElementAsRecord(i);

    const std::string& Lang = LangToSuffixes->getValueAsString("lang");
    const ListInit* Suffixes = LangToSuffixes->getValueAsListInit("suffixes");

    for (unsigned i = 0; i < Suffixes->size(); ++i)
      O << Indent1 << "language_map[\""
        << InitPtrToString(Suffixes->getElement(i))
        << "\"] = \"" << Lang << "\";\n";
  }

  O << "}\n\n";
}

/// FillInToolToLang - Fills in two tables that map tool names to
/// (input, output) languages.  Used by the typechecker.
void FillInToolToLang (const ToolPropertiesList& TPList,
                       StringMap<std::string>& ToolToInLang,
                       StringMap<std::string>& ToolToOutLang) {
  for (ToolPropertiesList::const_iterator B = TPList.begin(), E = TPList.end();
       B != E; ++B) {
    const ToolProperties& P = *(*B);
    ToolToInLang[P.Name] = P.InLanguage;
    ToolToOutLang[P.Name] = P.OutLanguage;
  }
}

/// TypecheckGraph - Check that names for output and input languages
/// on all edges do match.
// TOFIX: check for cycles.
// TOFIX: check for multiple default edges.
void TypecheckGraph (Record* CompilationGraph,
                     const ToolPropertiesList& TPList) {
  StringMap<std::string> ToolToInLang;
  StringMap<std::string> ToolToOutLang;

  FillInToolToLang(TPList, ToolToInLang, ToolToOutLang);
  ListInit* edges = CompilationGraph->getValueAsListInit("edges");
  StringMap<std::string>::iterator IAE = ToolToInLang.end();
  StringMap<std::string>::iterator IBE = ToolToOutLang.end();

  for (unsigned i = 0; i < edges->size(); ++i) {
    Record* Edge = edges->getElementAsRecord(i);
    Record* A = Edge->getValueAsDef("a");
    Record* B = Edge->getValueAsDef("b");
    StringMap<std::string>::iterator IA = ToolToOutLang.find(A->getName());
    StringMap<std::string>::iterator IB = ToolToInLang.find(B->getName());
    if(IA == IAE)
      throw A->getName() + ": no such tool!";
    if(IB == IBE)
      throw B->getName() + ": no such tool!";
    if(A->getName() != "root" && IA->second != IB->second)
      throw "Edge " + A->getName() + "->" + B->getName()
        + ": output->input language mismatch";
    if(B->getName() == "root")
      throw std::string("Edges back to the root are not allowed!");
  }
}

/// EmitEdgePropertyTest1Arg - Helper function used by
/// EmitEdgePropertyTest.
bool EmitEdgePropertyTest1Arg(const std::string& PropName,
                              const DagInit& Prop,
                              const GlobalOptionDescriptions& OptDescs,
                              std::ostream& O) {
  checkNumberOfArguments(&Prop, 1);
  const std::string& OptName = InitPtrToString(Prop.getArg(0));
  if (PropName == "switch_on") {
    const GlobalOptionDescription& OptDesc = OptDescs.FindOption(OptName);
    if (OptDesc.Type != OptionType::Switch)
      throw OptName + ": incorrect option type!";
    O << OptDesc.GenVariableName();
    return true;
  } else if (PropName == "if_input_languages_contain") {
    O << "InLangs.count(\"" << OptName << "\") != 0";
    return true;
  }

  return false;
}

/// EmitEdgePropertyTest2Args - Helper function used by
/// EmitEdgePropertyTest.
bool EmitEdgePropertyTest2Args(const std::string& PropName,
                               const DagInit& Prop,
                               const GlobalOptionDescriptions& OptDescs,
                               std::ostream& O) {
  checkNumberOfArguments(&Prop, 2);
  const std::string& OptName = InitPtrToString(Prop.getArg(0));
  const std::string& OptArg = InitPtrToString(Prop.getArg(1));
  const GlobalOptionDescription& OptDesc = OptDescs.FindOption(OptName);

  if (PropName == "parameter_equals") {
    if (OptDesc.Type != OptionType::Parameter
        && OptDesc.Type != OptionType::Prefix)
      throw OptName + ": incorrect option type!";
    O << OptDesc.GenVariableName() << " == \"" << OptArg << "\"";
    return true;
  }
  else if (PropName == "element_in_list") {
    if (OptDesc.Type != OptionType::ParameterList
        && OptDesc.Type != OptionType::PrefixList)
      throw OptName + ": incorrect option type!";
    const std::string& VarName = OptDesc.GenVariableName();
    O << "std::find(" << VarName << ".begin(),\n"
      << Indent3 << VarName << ".end(), \""
      << OptArg << "\") != " << VarName << ".end()";
    return true;
  }

  return false;
}

// Forward declaration.
void EmitEdgePropertyTest(const DagInit& Prop,
                          const GlobalOptionDescriptions& OptDescs,
                          std::ostream& O);

/// EmitLogicalOperationTest - Helper function used by
/// EmitEdgePropertyTest.
void EmitLogicalOperationTest(const DagInit& Prop, const char* LogicOp,
                              const GlobalOptionDescriptions& OptDescs,
                              std::ostream& O) {
  O << '(';
  for (unsigned j = 0, NumArgs = Prop.getNumArgs(); j < NumArgs; ++j) {
    const DagInit& InnerProp = InitPtrToDagInitRef(Prop.getArg(j));
    EmitEdgePropertyTest(InnerProp, OptDescs, O);
    if (j != NumArgs - 1)
      O << ")\n" << Indent3 << ' ' << LogicOp << " (";
    else
      O << ')';
  }
}

/// EmitEdgePropertyTest - Helper function used by EmitEdgeClass.
void EmitEdgePropertyTest(const DagInit& Prop,
                          const GlobalOptionDescriptions& OptDescs,
                          std::ostream& O) {
  const std::string& PropName = Prop.getOperator()->getAsString();

  if (PropName == "and")
    EmitLogicalOperationTest(Prop, "&&", OptDescs, O);
  else if (PropName == "or")
    EmitLogicalOperationTest(Prop, "||", OptDescs, O);
  else if (EmitEdgePropertyTest1Arg(PropName, Prop, OptDescs, O))
    return;
  else if (EmitEdgePropertyTest2Args(PropName, Prop, OptDescs, O))
    return;
  else
    throw PropName + ": unknown edge property!";
}

/// EmitEdgeClass - Emit a single Edge# class.
void EmitEdgeClass(unsigned N, const std::string& Target,
                   ListInit* Props, const GlobalOptionDescriptions& OptDescs,
                   std::ostream& O) {

  // Class constructor.
  O << "class Edge" << N << ": public Edge {\n"
    << "public:\n"
    << Indent1 << "Edge" << N << "() : Edge(\"" << Target
    << "\") {}\n\n"

  // Function Weight().
    << Indent1 << "unsigned Weight(const InputLanguagesSet& InLangs) const {\n"
    << Indent2 << "unsigned ret = 0;\n";

  // Emit tests for every edge property.
  for (size_t i = 0, PropsSize = Props->size(); i < PropsSize; ++i) {
    const DagInit& Prop = InitPtrToDagInitRef(Props->getElement(i));
    const std::string& PropName = Prop.getOperator()->getAsString();
    unsigned N = 2;

    O << Indent2 << "if (";

    if (PropName == "weight") {
      checkNumberOfArguments(&Prop, 2);
      N = InitPtrToInt(Prop.getArg(0));
      const DagInit& InnerProp = InitPtrToDagInitRef(Prop.getArg(1));
      EmitEdgePropertyTest(InnerProp, OptDescs, O);
    }
    else {
      EmitEdgePropertyTest(Prop, OptDescs, O);
    }

    O << ")\n" << Indent3 << "ret += " << N << ";\n";
  }

  O << Indent2 << "return ret;\n"
    << Indent1 << "};\n\n};\n\n";
}

// Emit Edge* classes that represent graph edges.
void EmitEdgeClasses (Record* CompilationGraph,
                      const GlobalOptionDescriptions& OptDescs,
                      std::ostream& O) {
  ListInit* edges = CompilationGraph->getValueAsListInit("edges");

  for (unsigned i = 0; i < edges->size(); ++i) {
    Record* Edge = edges->getElementAsRecord(i);
    Record* B = Edge->getValueAsDef("b");
    ListInit* Props = Edge->getValueAsListInit("props");

    if (Props->empty())
      continue;

    EmitEdgeClass(i, B->getName(), Props, OptDescs, O);
  }
}

/// EmitPopulateCompilationGraph - Emit the PopulateCompilationGraph()
/// function.
void EmitPopulateCompilationGraph (Record* CompilationGraph,
                                   std::ostream& O)
{
  ListInit* edges = CompilationGraph->getValueAsListInit("edges");

  // Generate code
  O << "void llvmc::PopulateCompilationGraph(CompilationGraph& G) {\n"
    << Indent1 << "PopulateLanguageMap(G.ExtsToLangs);\n\n";

  // Insert vertices

  RecordVector Tools = Records.getAllDerivedDefinitions("Tool");
  if (Tools.empty())
    throw std::string("No tool definitions found!");

  for (RecordVector::iterator B = Tools.begin(), E = Tools.end(); B != E; ++B) {
    const std::string& Name = (*B)->getName();
    if(Name != "root")
      O << Indent1 << "G.insertNode(new "
        << Name << "());\n";
  }

  O << '\n';

  // Insert edges
  for (unsigned i = 0; i < edges->size(); ++i) {
    Record* Edge = edges->getElementAsRecord(i);
    Record* A = Edge->getValueAsDef("a");
    Record* B = Edge->getValueAsDef("b");
    ListInit* Props = Edge->getValueAsListInit("props");

    O << Indent1 << "G.insertEdge(\"" << A->getName() << "\", ";

    if (Props->empty())
      O << "new SimpleEdge(\"" << B->getName() << "\")";
    else
      O << "new Edge" << i << "()";

    O << ");\n";
  }

  O << "}\n\n";
}


// End of anonymous namespace
}

/// run - The back-end entry point.
void LLVMCConfigurationEmitter::run (std::ostream &O) {

  // Emit file header.
  EmitSourceFileHeader("LLVMC Configuration Library", O);

  // Get a list of all defined Tools.
  RecordVector Tools = Records.getAllDerivedDefinitions("Tool");
  if (Tools.empty())
    throw std::string("No tool definitions found!");

  // Gather information from the Tool description dags.
  ToolPropertiesList tool_props;
  GlobalOptionDescriptions opt_descs;
  CollectToolProperties(Tools.begin(), Tools.end(), tool_props, opt_descs);

  // Emit global option registration code.
  EmitOptionDescriptions(opt_descs, O);

  // Emit PopulateLanguageMap() function
  // (a language map maps from file extensions to language names).
  EmitPopulateLanguageMap(Records, O);

  // Emit Tool classes.
  for (ToolPropertiesList::const_iterator B = tool_props.begin(),
         E = tool_props.end(); B!=E; ++B)
    EmitToolClassDefinition(*(*B), O);

  Record* CompilationGraphRecord = Records.getDef("CompilationGraph");
  if (!CompilationGraphRecord)
    throw std::string("Compilation graph description not found!");

  // Typecheck the compilation graph.
  TypecheckGraph(CompilationGraphRecord, tool_props);

  // Emit Edge# classes.
  EmitEdgeClasses(CompilationGraphRecord, opt_descs, O);

  // Emit PopulateCompilationGraph() function.
  EmitPopulateCompilationGraph(CompilationGraphRecord, O);

  // EOF
}