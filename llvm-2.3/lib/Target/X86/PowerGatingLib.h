	// No Op and other "fake" Instructions
	case X86::NOOP:
	case X86::PHI:
	case X86::INLINEASM:
	case X86::LABEL:
	case X86::DECLARE:
	case X86::EXTRACT_SUBREG:
	case X86::INSERT_SUBREG:
	case X86::IMPLICIT_DEF:
	case X86::SUBREG_TO_REG:
	case X86::ADJCALLSTACKDOWN:
	case X86::ADJCALLSTACKUP:
		return turnOnFUT(FUT_NONE);
		break;

	// Pushs
	case X86::PUSH32r:
	case X86::PUSH64r:
	case X86::PUSHFD:
	case X86::PUSHFQ:
		return turnOnFUT(2, FUT_STORE, FUT_AGU);
		break;

	// Pops
	case X86::POP32r:
	case X86::POP64r:
	case X86::POPFD:
	case X86::POPFQ:
		return turnOnFUT(2, FUT_LOAD, FUT_AGU);
		break;

	// Loads
	case X86::FsFLD0SD:
	case X86::FsFLD0SS:
	case X86::ILD_F16m: 
	case X86::ILD_F32m: 
	case X86::ILD_F64m:
	case X86::ILD_Fp16m32: 
	case X86::ILD_Fp16m64: 
	case X86::ILD_Fp16m80: 
	case X86::ILD_Fp32m32: 
	case X86::ILD_Fp32m64: 
	case X86::ILD_Fp32m80: 
	case X86::ILD_Fp64m32: 
	case X86::ILD_Fp64m64: 
	case X86::ILD_Fp64m80:
	case X86::LD_F0: 
	case X86::LD_F1: 
	case X86::LD_F32m: 
	case X86::LD_F64m: 
	case X86::LD_F80m: 
	case X86::LD_Fp032: 
	case X86::LD_Fp064: 
	case X86::LD_Fp080: 
	case X86::LD_Fp132: 
	case X86::LD_Fp164: 
	case X86::LD_Fp180: 
	case X86::LD_Fp32m: 
	case X86::LD_Fp32m64: 
	case X86::LD_Fp32m80: 
	case X86::LD_Fp64m: 
	case X86::LD_Fp64m80: 
	case X86::LD_Fp80m: 
	case X86::LD_Frr:
	case X86::FLDCW16m:
	case X86::LDDQUrm:
	case X86::LDMXCSR:
	case X86::RDTSC:
		return turnOnFUT(2, FUT_LOAD, FUT_AGU);
		break;

	// Stores
	case X86::ST_F32m:
	case X86::ST_F64m:
	case X86::ST_FP32m:
	case X86::ST_FP64m:
	case X86::ST_FP80m:
	case X86::ST_FPrr:
	case X86::ST_Fp32m:
	case X86::ST_Fp64m:
	case X86::ST_Fp64m32:
	case X86::ST_Fp80m32:
	case X86::ST_Fp80m64:
	case X86::ST_FpP32m:
	case X86::ST_FpP64m:
	case X86::ST_FpP64m32:
	case X86::ST_FpP80m:
	case X86::ST_FpP80m32:
	case X86::ST_FpP80m64:
	case X86::ST_Frr:
		return turnOnFUT(2, FUT_STORE, FUT_AGU);
		break;

	// Integer ALU Arithmetic ops
	case X86::ADC32mi: case X86::ADC32mi8: case X86::ADC32mr: case X86::ADC32ri: case X86::ADC32ri8: case X86::ADC32rm: case X86::ADC32rr: case X86::ADC64mi32: case X86::ADC64mi8: case X86::ADC64mr: case X86::ADC64ri32: case X86::ADC64ri8: case X86::ADC64rm: case X86::ADC64rr:
	case X86::ADD16mi: case X86::ADD16mi8: case X86::ADD16mr: case X86::ADD16ri: case X86::ADD16ri8: case X86::ADD16rm: case X86::ADD16rr: case X86::ADD32mi: case X86::ADD32mi8: case X86::ADD32mr: case X86::ADD32ri: case X86::ADD32ri8: case X86::ADD32rm: case X86::ADD32rr: case X86::ADD64mi32: case X86::ADD64mi8: case X86::ADD64mr: case X86::ADD64ri32: case X86::ADD64ri8: case X86::ADD64rm: case X86::ADD64rr: case X86::ADD8mi: case X86::ADD8mr: case X86::ADD8ri: case X86::ADD8rm: case X86::ADD8rr:
	case X86::ADDPDrm: case X86::ADDPDrr:
	case X86::ADDPSrm: case X86::ADDPSrr:
	case X86::ADDSDrm: case X86::ADDSDrm_Int:
	case X86::ADDSDrr: case X86::ADDSDrr_Int:
	case X86::ADDSSrm: case X86::ADDSSrm_Int:
	case X86::ADDSSrr: case X86::ADDSSrr_Int:
	case X86::ADDSUBPDrm: case X86::ADDSUBPDrr:
	case X86::ADDSUBPSrm: case X86::ADDSUBPSrr:
	case X86::DEC16m: case X86::DEC16r: case X86::DEC32m: case X86::DEC32r: case X86::DEC64_16m: case X86::DEC64_16r: case X86::DEC64_32m: case X86::DEC64_32r: case X86::DEC64m: case X86::DEC64r: case X86::DEC8m: case X86::DEC8r:
	case X86::SUB16mi: case X86::SUB16mi8: case X86::SUB16mr: case X86::SUB16ri: case X86::SUB16ri8: case X86::SUB16rm: case X86::SUB16rr: case X86::SUB32mi: case X86::SUB32mi8: case X86::SUB32mr: case X86::SUB32ri: case X86::SUB32ri8: case X86::SUB32rm: case X86::SUB32rr: case X86::SUB64mi32: case X86::SUB64mi8: case X86::SUB64mr: case X86::SUB64ri32: case X86::SUB64ri8: case X86::SUB64rm: case X86::SUB64rr: case X86::SUB8mi: case X86::SUB8mr: case X86::SUB8ri: case X86::SUB8rm: case X86::SUB8rr:
	case X86::SUBPDrm: case X86::SUBPDrr:
	case X86::SUBPSrm: case X86::SUBPSrr:
	case X86::SUBSDrm:
	case X86::SUBSDrm_Int:
	case X86::SUBSDrr:
	case X86::SUBSDrr_Int:
	case X86::SUBSSrm:
	case X86::SUBSSrm_Int:
	case X86::SUBSSrr:
	case X86::SUBSSrr_Int:
	case X86::INC16m: case X86::INC16r: case X86::INC32m: case X86::INC32r: case X86::INC64_16m: case X86::INC64_16r: case X86::INC64_32m: case X86::INC64_32r: case X86::INC64m: case X86::INC64r: case X86::INC8m: case X86::INC8r:
		return turnOnFUT(4, FUT_INT_ADD_ARITH, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Integer ALU Logical ops
	case X86::NEG16m:
	case X86::NEG16r:
	case X86::NEG32m:
	case X86::NEG32r:
	case X86::NEG64m:
	case X86::NEG64r:
	case X86::NEG8m:
	case X86::NEG8r:
	case X86::NOT16m:
	case X86::NOT16r:
	case X86::NOT32m:
	case X86::NOT32r:
	case X86::NOT64m:
	case X86::NOT64r:
	case X86::NOT8m:
	case X86::NOT8r:
	case X86::OR16mi:
	case X86::OR16mi8:
	case X86::OR16mr:
	case X86::OR16ri:
	case X86::OR16ri8:
	case X86::OR16rm:
	case X86::OR16rr:
	case X86::OR32mi:
	case X86::OR32mi8:
	case X86::OR32mr:
	case X86::OR32ri:
	case X86::OR32ri8:
	case X86::OR32rm:
	case X86::OR32rr:
	case X86::OR64mi32:
	case X86::OR64mi8:
	case X86::OR64mr:
	case X86::OR64ri32:
	case X86::OR64ri8:
	case X86::OR64rm:
	case X86::OR64rr:
	case X86::OR8mi:
	case X86::OR8mr:
	case X86::OR8ri:
	case X86::OR8rm:
	case X86::OR8rr:
	case X86::ORPDrm:
	case X86::ORPDrr:
	case X86::ORPSrm:
	case X86::ORPSrr:
	case X86::AND16mi: case X86::AND16mi8: case X86::AND16mr: case X86::AND16ri: case X86::AND16ri8: case X86::AND16rm: case X86::AND16rr: case X86::AND32mi: case X86::AND32mi8: case X86::AND32mr: case X86::AND32ri: case X86::AND32ri8: case X86::AND32rm: case X86::AND32rr: case X86::AND64mi32: case X86::AND64mi8: case X86::AND64mr: case X86::AND64ri32: case X86::AND64ri8: case X86::AND64rm: case X86::AND64rr: case X86::AND8mi: case X86::AND8mr: case X86::AND8ri: case X86::AND8rm: case X86::AND8rr:
	case X86::ANDNPDrm: case X86::ANDNPDrr:
	case X86::ANDNPSrm: case X86::ANDNPSrr:
	case X86::ANDPDrm: case X86::ANDPDrr:
	case X86::ANDPSrm: case X86::ANDPSrr:
	case X86::FsANDNPDrm: case X86::FsANDNPDrr:
	case X86::FsANDNPSrm: case X86::FsANDNPSrr:
	case X86::FsANDPDrm: case X86::FsANDPDrr:
	case X86::FsANDPSrm: case X86::FsANDPSrr:
	case X86::FsORPDrm: case X86::FsORPDrr:
	case X86::FsORPSrm: case X86::FsORPSrr:
	case X86::FsXORPDrm: case X86::FsXORPDrr:
	case X86::FsXORPSrm: case X86::FsXORPSrr:
	case X86::XOR16mi: case X86::XOR16mi8: case X86::XOR16mr: case X86::XOR16ri: case X86::XOR16ri8: case X86::XOR16rm: case X86::XOR16rr: case X86::XOR32mi: case X86::XOR32mi8: case X86::XOR32mr: case X86::XOR32ri: case X86::XOR32ri8: case X86::XOR32rm: case X86::XOR32rr: case X86::XOR64mi32: case X86::XOR64mi8: case X86::XOR64mr: case X86::XOR64ri32: case X86::XOR64ri8: case X86::XOR64rm: case X86::XOR64rr: case X86::XOR8mi: case X86::XOR8mr: case X86::XOR8ri: case X86::XOR8rm: case X86::XOR8rr:
	case X86::XORPDrm: case X86::XORPDrr:
	case X86::XORPSrm: case X86::XORPSrr:
		return turnOnFUT(4, FUT_INT_ADD_LOGIC, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Integer Multiply
	case X86::IMUL16m: case X86::IMUL16r: case X86::IMUL16rm: case X86::IMUL16rmi: case X86::IMUL16rmi8: case X86::IMUL16rr: case X86::IMUL16rri: case X86::IMUL16rri8: case X86::IMUL32m: case X86::IMUL32r: case X86::IMUL32rm: case X86::IMUL32rmi: case X86::IMUL32rmi8: case X86::IMUL32rr: case X86::IMUL32rri: case X86::IMUL32rri8: case X86::IMUL64m: case X86::IMUL64r: case X86::IMUL64rm: case X86::IMUL64rmi32: case X86::IMUL64rmi8: case X86::IMUL64rr: case X86::IMUL64rri32: case X86::IMUL64rri8: case X86::IMUL8m: case X86::IMUL8r:
	case X86::MUL16m:
	case X86::MUL16r:
	case X86::MUL32m:
	case X86::MUL32r:
	case X86::MUL64m:
	case X86::MUL64r:
	case X86::MUL8m:
	case X86::MUL8r:
	case X86::MULPDrm:
	case X86::MULPDrr:
	case X86::MULPSrm:
	case X86::MULPSrr:
	case X86::MULSDrm:
	case X86::MULSDrm_Int:
	case X86::MULSDrr:
	case X86::MULSDrr_Int:
	case X86::MULSSrm:
	case X86::MULSSrm_Int:
	case X86::MULSSrr:
	case X86::MULSSrr_Int:
		return turnOnFUT(4, FUT_INT_MUL, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;
	
	// Integer Divide
	case X86::IDIV16m: case X86::IDIV16r: case X86::IDIV32m: case X86::IDIV32r: case X86::IDIV64m: case X86::IDIV64r: case X86::IDIV8m: case X86::IDIV8r:
	case X86::DIV16m: case X86::DIV16r: case X86::DIV32m: case X86::DIV32r: case X86::DIV64m: case X86::DIV64r: case X86::DIV8m: case X86::DIV8r:
	case X86::DIVPDrm: case X86::DIVPDrr:
	case X86::DIVPSrm: case X86::DIVPSrr:
	case X86::DIVSDrm: case X86::DIVSDrm_Int: case X86::DIVSDrr: case X86::DIVSDrr_Int:
	case X86::DIVSSrm: case X86::DIVSSrm_Int: case X86::DIVSSrr: case X86::DIVSSrr_Int:
		return turnOnFUT(4, FUT_INT_DIV, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Floating Point ALU Arithmetic ops
	case X86::ADD_F32m: case X86::ADD_F64m:
	case X86::ADD_FI16m: case X86::ADD_FI32m:
	case X86::ADD_FPrST0:
	case X86::ADD_FST0r:
	case X86::ADD_Fp32: case X86::ADD_Fp32m:
	case X86::ADD_Fp64: case X86::ADD_Fp64m:
	case X86::ADD_Fp64m32:
	case X86::ADD_Fp80: case X86::ADD_Fp80m32: case X86::ADD_Fp80m64:
	case X86::ADD_FpI16m32: case X86::ADD_FpI16m64: case X86::ADD_FpI16m80: case X86::ADD_FpI32m32: case X86::ADD_FpI32m64: case X86::ADD_FpI32m80:
	case X86::ADD_FrST0:
	case X86::FP32_TO_INT16_IN_MEM: case X86::FP32_TO_INT32_IN_MEM: case X86::FP32_TO_INT64_IN_MEM: case X86::FP64_TO_INT16_IN_MEM: case X86::FP64_TO_INT32_IN_MEM: case X86::FP64_TO_INT64_IN_MEM: case X86::FP80_TO_INT16_IN_MEM: case X86::FP80_TO_INT32_IN_MEM: case X86::FP80_TO_INT64_IN_MEM:
	case X86::FP_REG_KILL:
	case X86::FpGET_ST0_32: case X86::FpGET_ST0_64: case X86::FpGET_ST0_80: case X86::FpGET_ST1_32: case X86::FpGET_ST1_64: case X86::FpGET_ST1_80: case X86::FpSET_ST0_32: case X86::FpSET_ST0_64: case X86::FpSET_ST0_80:
	case X86::SUBR_F32m: case X86::SUBR_F64m: case X86::SUBR_FI16m: case X86::SUBR_FI32m: case X86::SUBR_FPrST0: case X86::SUBR_FST0r: case X86::SUBR_Fp32m: case X86::SUBR_Fp64m: case X86::SUBR_Fp64m32: case X86::SUBR_Fp80m32: case X86::SUBR_Fp80m64: case X86::SUBR_FpI16m32: case X86::SUBR_FpI16m64: case X86::SUBR_FpI16m80: case X86::SUBR_FpI32m32: case X86::SUBR_FpI32m64: case X86::SUBR_FpI32m80: case X86::SUBR_FrST0:
	case X86::SUB_F32m: case X86::SUB_F64m: case X86::SUB_FI16m: case X86::SUB_FI32m: case X86::SUB_FPrST0: case X86::SUB_FST0r: case X86::SUB_Fp32: case X86::SUB_Fp32m: case X86::SUB_Fp64: case X86::SUB_Fp64m: case X86::SUB_Fp64m32: case X86::SUB_Fp80: case X86::SUB_Fp80m32: case X86::SUB_Fp80m64: case X86::SUB_FpI16m32: case X86::SUB_FpI16m64: case X86::SUB_FpI16m80: case X86::SUB_FpI32m32: case X86::SUB_FpI32m64: case X86::SUB_FpI32m80: case X86::SUB_FrST0:
		return turnOnFUT(4, FUT_FP_ADD, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Floating Point Multiply
	case X86::MUL_F32m:
	case X86::MUL_F64m:
	case X86::MUL_FI16m:
	case X86::MUL_FI32m:
	case X86::MUL_FPrST0:
	case X86::MUL_FST0r:
	case X86::MUL_Fp32:
	case X86::MUL_Fp32m:
	case X86::MUL_Fp64:
	case X86::MUL_Fp64m:
	case X86::MUL_Fp64m32:
	case X86::MUL_Fp80:
	case X86::MUL_Fp80m32:
	case X86::MUL_Fp80m64:
	case X86::MUL_FpI16m32:
	case X86::MUL_FpI16m64:
	case X86::MUL_FpI16m80:
	case X86::MUL_FpI32m32:
	case X86::MUL_FpI32m64:
	case X86::MUL_FpI32m80:
	case X86::MUL_FrST0:
		return turnOnFUT(4, FUT_FP_MUL, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Floating Point Divide
	case X86::DIVR_F32m:
	case X86::DIVR_F64m:
	case X86::DIVR_FI16m:
	case X86::DIVR_FI32m:
	case X86::DIVR_FPrST0:
	case X86::DIVR_FST0r:
	case X86::DIVR_Fp32m: case X86::DIVR_Fp64m: case X86::DIVR_Fp64m32: case X86::DIVR_Fp80m32: case X86::DIVR_Fp80m64:
	case X86::DIVR_FpI16m32: case X86::DIVR_FpI16m64: case X86::DIVR_FpI16m80: case X86::DIVR_FpI32m32: case X86::DIVR_FpI32m64: case X86::DIVR_FpI32m80: case X86::DIVR_FrST0:
	case X86::DIV_F32m:
	case X86::DIV_F64m:
	case X86::DIV_FI16m:
	case X86::DIV_FI32m:
	case X86::DIV_FPrST0:
	case X86::DIV_FST0r:
	case X86::DIV_Fp32: case X86::DIV_Fp32m: case X86::DIV_Fp64: case X86::DIV_Fp64m: case X86::DIV_Fp64m32: case X86::DIV_Fp80: case X86::DIV_Fp80m32: case X86::DIV_Fp80m64:
	case X86::DIV_FpI16m32: case X86::DIV_FpI16m64: case X86::DIV_FpI16m80: case X86::DIV_FpI32m32: case X86::DIV_FpI32m64: case X86::DIV_FpI32m80:
	case X86::DIV_FrST0:
		return turnOnFUT(4, FUT_FP_DIV, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Calls
	case X86::CALL32m:
	case X86::CALL32r:
	case X86::CALL64m:
	case X86::CALL64pcrel32:
	case X86::CALL64r:
	case X86::CALLpcrel32:
	case X86::RET:
	case X86::RETI:
		return turnOnFUT(1, FUT_BRANCH);
		break;

	// Moves
	case X86::CMOVA16rm: case X86::CMOVA16rr: case X86::CMOVA32rm: case X86::CMOVA32rr: case X86::CMOVA64rm: case X86::CMOVA64rr:
	case X86::CMOVAE16rm: case X86::CMOVAE16rr: case X86::CMOVAE32rm: case X86::CMOVAE32rr: case X86::CMOVAE64rm: case X86::CMOVAE64rr:
	case X86::CMOVB16rm: case X86::CMOVB16rr: case X86::CMOVB32rm: case X86::CMOVB32rr: case X86::CMOVB64rm: case X86::CMOVB64rr:
	case X86::CMOVBE16rm: case X86::CMOVBE16rr: case X86::CMOVBE32rm: case X86::CMOVBE32rr: case X86::CMOVBE64rm: case X86::CMOVBE64rr:
	case X86::CMOVBE_F: case X86::CMOVBE_Fp32: case X86::CMOVBE_Fp64: case X86::CMOVBE_Fp80:
	case X86::CMOVB_F: case X86::CMOVB_Fp32: case X86::CMOVB_Fp64: case X86::CMOVB_Fp80:
	case X86::CMOVE16rm: case X86::CMOVE16rr: case X86::CMOVE32rm: case X86::CMOVE32rr: case X86::CMOVE64rm: case X86::CMOVE64rr:
	case X86::CMOVE_F: case X86::CMOVE_Fp32: case X86::CMOVE_Fp64: case X86::CMOVE_Fp80:
	case X86::CMOVG16rm: case X86::CMOVG16rr: case X86::CMOVG32rm: case X86::CMOVG32rr: case X86::CMOVG64rm: case X86::CMOVG64rr:
	case X86::CMOVGE16rm: case X86::CMOVGE16rr: case X86::CMOVGE32rm: case X86::CMOVGE32rr: case X86::CMOVGE64rm: case X86::CMOVGE64rr:
	case X86::CMOVL16rm: case X86::CMOVL16rr: case X86::CMOVL32rm: case X86::CMOVL32rr: case X86::CMOVL64rm: case X86::CMOVL64rr:
	case X86::CMOVLE16rm: case X86::CMOVLE16rr: case X86::CMOVLE32rm: case X86::CMOVLE32rr: case X86::CMOVLE64rm: case X86::CMOVLE64rr:
	case X86::CMOVNBE_F: case X86::CMOVNBE_Fp32: case X86::CMOVNBE_Fp64: case X86::CMOVNBE_Fp80:
	case X86::CMOVNB_F: case X86::CMOVNB_Fp32: case X86::CMOVNB_Fp64: case X86::CMOVNB_Fp80:
	case X86::CMOVNE16rm: case X86::CMOVNE16rr: case X86::CMOVNE32rm: case X86::CMOVNE32rr: case X86::CMOVNE64rm: case X86::CMOVNE64rr:
	case X86::CMOVNE_F: case X86::CMOVNE_Fp32: case X86::CMOVNE_Fp64: case X86::CMOVNE_Fp80:
	case X86::CMOVNP16rm: case X86::CMOVNP16rr: case X86::CMOVNP32rm: case X86::CMOVNP32rr: case X86::CMOVNP64rm: case X86::CMOVNP64rr:
	case X86::CMOVNP_F: case X86::CMOVNP_Fp32: case X86::CMOVNP_Fp64: case X86::CMOVNP_Fp80:
	case X86::CMOVNS16rm: case X86::CMOVNS16rr: case X86::CMOVNS32rm: case X86::CMOVNS32rr: case X86::CMOVNS64rm: case X86::CMOVNS64rr:
	case X86::CMOVP16rm: case X86::CMOVP16rr: case X86::CMOVP32rm: case X86::CMOVP32rr: case X86::CMOVP64rm: case X86::CMOVP64rr:
	case X86::CMOVP_F: case X86::CMOVP_Fp32: case X86::CMOVP_Fp64: case X86::CMOVP_Fp80:
	case X86::CMOVS16rm: case X86::CMOVS16rr: case X86::CMOVS32rm: case X86::CMOVS32rr: case X86::CMOVS64rm: case X86::CMOVS64rr:
	case X86::CMOV_FR32: case X86::CMOV_FR64: case X86::CMOV_V2F64:
	case X86::CMOV_V2I64: case X86::CMOV_V4F32:
	case X86::FsMOVAPDrm: case X86::FsMOVAPDrr:
	case X86::FsMOVAPSrm: case X86::FsMOVAPSrr:
		return turnOnFUT(4, FUT_LOAD, FUT_STORE, FUT_CMP, FUT_AGU);
		break;

	// Compares
	case X86::CMP16mi: case X86::CMP16mi8: case X86::CMP16mr: case X86::CMP16ri: case X86::CMP16ri8: case X86::CMP16rm: case X86::CMP16rr: case X86::CMP32mi: case X86::CMP32mi8: case X86::CMP32mr: case X86::CMP32ri: case X86::CMP32ri8: case X86::CMP32rm: case X86::CMP32rr: case X86::CMP64mi32: case X86::CMP64mi8: case X86::CMP64mr: case X86::CMP64ri32: case X86::CMP64ri8: case X86::CMP64rm: case X86::CMP64rr: case X86::CMP8mi: case X86::CMP8mr: case X86::CMP8ri: case X86::CMP8rm: case X86::CMP8rr:
	case X86::CMPPDrmi: case X86::CMPPDrri:
	case X86::CMPPSrmi: case X86::CMPPSrri:
	case X86::CMPSDrm: case X86::CMPSDrr:
	case X86::CMPSSrm: case X86::CMPSSrr:
		return turnOnFUT(4, FUT_CMP, FUT_STORE, FUT_LOAD, FUT_AGU);
		break;

	// Input From Port
	case X86::IN16ri: 
	case X86::IN16rr: 
	case X86::IN32ri: 
	case X86::IN32rr: 
	case X86::IN8ri: 
	case X86::IN8rr:
		return turnOnFUT(2, FUT_LOAD, FUT_AGU);
		break;

	// Branches and Jumps
	case X86::TAILCALL:
	case X86::TAILJMPd:
	case X86::TAILJMPr:
	case X86::TAILJMPr64:
	case X86::JA:
	case X86::JAE:
	case X86::JB:
	case X86::JBE:
	case X86::JE:
	case X86::JG:
	case X86::JGE:
	case X86::JL:
	case X86::JLE:
	case X86::JMP:
	case X86::JMP32r: 
	case X86::JMP64r:
	case X86::JNE:
	case X86::JNO:
	case X86::JNP:
	case X86::JNS:
	case X86::JO:
	case X86::JP:
	case X86::JS:
		return turnOnFUT(1, FUT_BRANCH);
		break;

	// Branch with Memory
	case X86::TAILJMPm:
	case X86::JMP32m: 
	case X86::JMP64m: 
		return turnOnFUT(3, FUT_BRANCH, FUT_LOAD, FUT_AGU);
		break;

	// Load effective address
	case X86::LEA16r:
	case X86::LEA32r:
	case X86::LEA64_32r:
	case X86::LEA64r:
		return turnOnFUT(1, FUT_AGU);
		break;

	// Moves
	case X86::MOV16_mr:
	case X86::MOV16_rm:
	case X86::MOV16_rr:
	case X86::MOV16mi:
	case X86::MOV16mr:
	case X86::MOV16r0:
	case X86::MOV16ri:
	case X86::MOV16rm:
	case X86::MOV16rr:
	case X86::MOV16to16_:
	case X86::MOV32_mr:
	case X86::MOV32_rm:
	case X86::MOV32_rr:
	case X86::MOV32mi:
	case X86::MOV32mr:
	case X86::MOV32r0:
	case X86::MOV32ri:
	case X86::MOV32rm:
	case X86::MOV32rr:
	case X86::MOV32to32_:
	case X86::MOV64mi32:
	case X86::MOV64mr:
	case X86::MOV64r0:
	case X86::MOV64ri:
	case X86::MOV64ri32:
	case X86::MOV64ri64i32:
	case X86::MOV64rm:
	case X86::MOV64rr:
	case X86::MOV64toPQIrr:
	case X86::MOV64toSDrm:
	case X86::MOV64toSDrr:
	case X86::MOV8mi:
	case X86::MOV8mr:
	case X86::MOV8r0:
	case X86::MOV8ri:
	case X86::MOV8rm:
	case X86::MOV8rr:
	case X86::MOVAPDmr:
	case X86::MOVAPDrm:
	case X86::MOVAPDrr:
	case X86::MOVAPSmr:
	case X86::MOVAPSrm:
	case X86::MOVAPSrr:
	case X86::MOVDDUPrm:
	case X86::MOVDDUPrr:
	case X86::MOVDI2PDIrm:
	case X86::MOVDI2PDIrr:
	case X86::MOVDI2SSrm:
	case X86::MOVDI2SSrr:
	case X86::MOVDQAmr:
	case X86::MOVDQArm:
	case X86::MOVDQArr:
	case X86::MOVDQUmr:
	case X86::MOVDQUmr_Int:
	case X86::MOVDQUrm:
	case X86::MOVDQUrm_Int:
	case X86::MOVHLPSrr:
	case X86::MOVHPDmr:
	case X86::MOVHPDrm:
	case X86::MOVHPSmr:
	case X86::MOVHPSrm:
	case X86::MOVLHPSrr:
	case X86::MOVLPDmr:
	case X86::MOVLPDrm:
	case X86::MOVLPDrr:
	case X86::MOVLPSmr:
	case X86::MOVLPSrm:
	case X86::MOVLPSrr:
	case X86::MOVLQ128mr:
	case X86::MOVLSD2PDrr:
	case X86::MOVLSS2PSrr:
	case X86::MOVMSKPDrr:
	case X86::MOVMSKPSrr:
	case X86::MOVNTDQArm:
	case X86::MOVNTDQmr:
	case X86::MOVNTImr:
	case X86::MOVNTPDmr:
	case X86::MOVNTPSmr:
	case X86::MOVPC32r:
	case X86::MOVPD2SDmr:
	case X86::MOVPD2SDrr:
	case X86::MOVPDI2DImr:
	case X86::MOVPDI2DIrr:
	case X86::MOVPQI2QImr:
	case X86::MOVPQIto64rr:
	case X86::MOVPS2SSmr:
	case X86::MOVPS2SSrr:
	case X86::MOVQI2PQIrm:
	case X86::MOVSD2PDrm:
	case X86::MOVSD2PDrr:
	case X86::MOVSDmr:
	case X86::MOVSDrm:
	case X86::MOVSDrr:
	case X86::MOVSDto64mr:
	case X86::MOVSDto64rr:
	case X86::MOVSHDUPrm:
	case X86::MOVSHDUPrr:
	case X86::MOVSLDUPrm:
	case X86::MOVSLDUPrr:
	case X86::MOVSS2DImr:
	case X86::MOVSS2DIrr:
	case X86::MOVSS2PSrm:
	case X86::MOVSS2PSrr:
	case X86::MOVSSmr:
	case X86::MOVSSrm:
	case X86::MOVSSrr:
	case X86::MOVSX16rm8:
	case X86::MOVSX16rr8:
	case X86::MOVSX32rm16:
	case X86::MOVSX32rm8:
	case X86::MOVSX32rr16:
	case X86::MOVSX32rr8:
	case X86::MOVSX64rm16:
	case X86::MOVSX64rm32:
	case X86::MOVSX64rm8:
	case X86::MOVSX64rr16:
	case X86::MOVSX64rr32:
	case X86::MOVSX64rr8:
	case X86::MOVUPDmr:
	case X86::MOVUPDmr_Int:
	case X86::MOVUPDrm:
	case X86::MOVUPDrm_Int:
	case X86::MOVUPDrr:
	case X86::MOVUPSmr:
	case X86::MOVUPSmr_Int:
	case X86::MOVUPSrm:
	case X86::MOVUPSrm_Int:
	case X86::MOVUPSrr:
	case X86::MOVZDI2PDIrm:
	case X86::MOVZDI2PDIrr:
	case X86::MOVZPQILo2PQIrm:
	case X86::MOVZPQILo2PQIrr:
	case X86::MOVZQI2PQIrm:
	case X86::MOVZQI2PQIrr:
	case X86::MOVZSD2PDrm:
	case X86::MOVZSS2PSrm:
	case X86::MOVZX16rm8:
	case X86::MOVZX16rr8:
	case X86::MOVZX32rm16:
	case X86::MOVZX32rm8:
	case X86::MOVZX32rr16:
	case X86::MOVZX32rr8:
	case X86::MOVZX64rm16:
	case X86::MOVZX64rm8:
	case X86::MOVZX64rr16:
	case X86::MOVZX64rr8:
	case X86::MOV_Fp3232:
	case X86::MOV_Fp3264:
	case X86::MOV_Fp3280:
	case X86::MOV_Fp6432:
	case X86::MOV_Fp6464:
	case X86::MOV_Fp6480:
	case X86::MOV_Fp8032:
	case X86::MOV_Fp8064:
	case X86::MOV_Fp8080:
		return turnOnFUT(4, FUT_MOVE, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Sets
	case X86::SETAEm:
	case X86::SETAEr:
	case X86::SETAm:
	case X86::SETAr:
	case X86::SETBEm:
	case X86::SETBEr:
	case X86::SETBm:
	case X86::SETBr:
	case X86::SETEm:
	case X86::SETEr:
	case X86::SETGEm:
	case X86::SETGEr:
	case X86::SETGm:
	case X86::SETGr:
	case X86::SETLEm:
	case X86::SETLEr:
	case X86::SETLm:
	case X86::SETLr:
	case X86::SETNEm:
	case X86::SETNEr:
	case X86::SETNPm:
	case X86::SETNPr:
	case X86::SETNSm:
	case X86::SETNSr:
	case X86::SETPm:
	case X86::SETPr:
	case X86::SETSm:
	case X86::SETSr:
		return turnOnFUT(4, FUT_SET, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Test Operands ops
	case X86::TEST16mi: 
	case X86::TEST16ri: 
	case X86::TEST16rm: 
	case X86::TEST16rr: 
	case X86::TEST32mi: 
	case X86::TEST32ri: 
	case X86::TEST32rm: 
	case X86::TEST32rr: 
	case X86::TEST64mi32: 
	case X86::TEST64ri32: 
	case X86::TEST64rm: 
	case X86::TEST64rr: 
	case X86::TEST8mi: 
	case X86::TEST8ri: 
	case X86::TEST8rm: 
	case X86::TEST8rr:
		return turnOnFUT(4, FUT_CMP, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// CVT ops - convert single to double precision FP
	case X86::CVTSD2SSrm:
	case X86::CVTSD2SSrr:
	case X86::CVTSI2SD64rm:
	case X86::CVTSI2SD64rr:
	case X86::CVTSI2SDrm:
	case X86::CVTSI2SDrr:
	case X86::CVTSI2SS64rm:
	case X86::CVTSI2SS64rr:
	case X86::CVTSI2SSrm:
	case X86::CVTSI2SSrr:
	case X86::CVTSS2SDrm:
	case X86::CVTSS2SDrr:
	case X86::CVTTSD2SI64rm:
	case X86::CVTTSD2SI64rr:
	case X86::CVTTSD2SIrm:
	case X86::CVTTSD2SIrr:
	case X86::CVTTSS2SI64rm:
	case X86::CVTTSS2SI64rr:
	case X86::CVTTSS2SIrm:
	case X86::CVTTSS2SIrr:
		return turnOnFUT(4, FUT_FP_ADD, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Integer CVT ops
	case X86::Int_CVTDQ2PDrm:
	case X86::Int_CVTDQ2PDrr:
	case X86::Int_CVTDQ2PSrm:
	case X86::Int_CVTDQ2PSrr:
	case X86::Int_CVTPD2DQrm:
	case X86::Int_CVTPD2DQrr:
	case X86::Int_CVTPD2PIrm:
	case X86::Int_CVTPD2PIrr:
	case X86::Int_CVTPD2PSrm:
	case X86::Int_CVTPD2PSrr:
	case X86::Int_CVTPI2PDrm:
	case X86::Int_CVTPI2PDrr:
	case X86::Int_CVTPI2PSrm:
	case X86::Int_CVTPI2PSrr:
	case X86::Int_CVTPS2DQrm:
	case X86::Int_CVTPS2DQrr:
	case X86::Int_CVTPS2PDrm:
	case X86::Int_CVTPS2PDrr:
	case X86::Int_CVTPS2PIrm:
	case X86::Int_CVTPS2PIrr:
	case X86::Int_CVTSD2SI64rm:
	case X86::Int_CVTSD2SI64rr:
	case X86::Int_CVTSD2SIrm:
	case X86::Int_CVTSD2SIrr:
	case X86::Int_CVTSD2SSrm:
	case X86::Int_CVTSD2SSrr:
	case X86::Int_CVTSI2SD64rm:
	case X86::Int_CVTSI2SD64rr:
	case X86::Int_CVTSI2SDrm:
	case X86::Int_CVTSI2SDrr:
	case X86::Int_CVTSI2SS64rm:
	case X86::Int_CVTSI2SS64rr:
	case X86::Int_CVTSI2SSrm:
	case X86::Int_CVTSI2SSrr:
	case X86::Int_CVTSS2SDrm:
	case X86::Int_CVTSS2SDrr:
	case X86::Int_CVTSS2SI64rm:
	case X86::Int_CVTSS2SI64rr:
	case X86::Int_CVTSS2SIrm:
	case X86::Int_CVTSS2SIrr:
	case X86::Int_CVTTPD2DQrm:
	case X86::Int_CVTTPD2DQrr:
	case X86::Int_CVTTPD2PIrm:
	case X86::Int_CVTTPD2PIrr:
	case X86::Int_CVTTPS2DQrm:
	case X86::Int_CVTTPS2DQrr:
	case X86::Int_CVTTPS2PIrm:
	case X86::Int_CVTTPS2PIrr:
	case X86::Int_CVTTSD2SI64rm:
	case X86::Int_CVTTSD2SI64rr:
	case X86::Int_CVTTSD2SIrm:
	case X86::Int_CVTTSD2SIrr:
	case X86::Int_CVTTSS2SI64rm:
	case X86::Int_CVTTSS2SI64rr:
	case X86::Int_CVTTSS2SIrm:
	case X86::Int_CVTTSS2SIrr:
		return turnOnFUT(4, FUT_INT_ADD_ARITH, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// MMX ops
	case X86::MMX_CVTPD2PIrm:
	case X86::MMX_CVTPD2PIrr:
	case X86::MMX_CVTPI2PDrm:
	case X86::MMX_CVTPI2PDrr:
	case X86::MMX_CVTPI2PSrm:
	case X86::MMX_CVTPI2PSrr:
	case X86::MMX_CVTPS2PIrm:
	case X86::MMX_CVTPS2PIrr:
	case X86::MMX_CVTTPD2PIrm:
	case X86::MMX_CVTTPD2PIrr:
	case X86::MMX_CVTTPS2PIrm:
	case X86::MMX_CVTTPS2PIrr:
	case X86::MMX_EMMS:
	case X86::MMX_FEMMS:
	case X86::MMX_MASKMOVQ:
	case X86::MMX_MOVD64from64rr:
	case X86::MMX_MOVD64mr:
	case X86::MMX_MOVD64rm:
	case X86::MMX_MOVD64rr:
	case X86::MMX_MOVD64to64rr:
	case X86::MMX_MOVDQ2Qrr:
	case X86::MMX_MOVNTQmr:
	case X86::MMX_MOVQ2DQrr:
	case X86::MMX_MOVQ64mr:
	case X86::MMX_MOVQ64rm:
	case X86::MMX_MOVQ64rr:
	case X86::MMX_MOVZDI2PDIrm:
	case X86::MMX_MOVZDI2PDIrr:
	case X86::MMX_PACKSSDWrm:
	case X86::MMX_PACKSSDWrr:
	case X86::MMX_PACKSSWBrm:
	case X86::MMX_PACKSSWBrr:
	case X86::MMX_PACKUSWBrm:
	case X86::MMX_PACKUSWBrr:
	case X86::MMX_PADDBrm:
	case X86::MMX_PADDBrr:
	case X86::MMX_PADDDrm:
	case X86::MMX_PADDDrr:
	case X86::MMX_PADDQrm:
	case X86::MMX_PADDQrr:
	case X86::MMX_PADDSBrm:
	case X86::MMX_PADDSBrr:
	case X86::MMX_PADDSWrm:
	case X86::MMX_PADDSWrr:
	case X86::MMX_PADDUSBrm:
	case X86::MMX_PADDUSBrr:
	case X86::MMX_PADDUSWrm:
	case X86::MMX_PADDUSWrr:
	case X86::MMX_PADDWrm:
	case X86::MMX_PADDWrr:
	case X86::MMX_PANDNrm:
	case X86::MMX_PANDNrr:
	case X86::MMX_PANDrm:
	case X86::MMX_PANDrr:
	case X86::MMX_PAVGBrm:
	case X86::MMX_PAVGBrr:
	case X86::MMX_PAVGWrm:
	case X86::MMX_PAVGWrr:
	case X86::MMX_PCMPEQBrm:
	case X86::MMX_PCMPEQBrr:
	case X86::MMX_PCMPEQDrm:
	case X86::MMX_PCMPEQDrr:
	case X86::MMX_PCMPEQWrm:
	case X86::MMX_PCMPEQWrr:
	case X86::MMX_PCMPGTBrm:
	case X86::MMX_PCMPGTBrr:
	case X86::MMX_PCMPGTDrm:
	case X86::MMX_PCMPGTDrr:
	case X86::MMX_PCMPGTWrm:
	case X86::MMX_PCMPGTWrr:
	case X86::MMX_PEXTRWri:
	case X86::MMX_PINSRWrmi:
	case X86::MMX_PINSRWrri:
	case X86::MMX_PMADDWDrm:
	case X86::MMX_PMADDWDrr:
	case X86::MMX_PMAXSWrm:
	case X86::MMX_PMAXSWrr:
	case X86::MMX_PMAXUBrm:
	case X86::MMX_PMAXUBrr:
	case X86::MMX_PMINSWrm:
	case X86::MMX_PMINSWrr:
	case X86::MMX_PMINUBrm:
	case X86::MMX_PMINUBrr:
	case X86::MMX_PMOVMSKBrr:
	case X86::MMX_PMULHUWrm:
	case X86::MMX_PMULHUWrr:
	case X86::MMX_PMULHWrm:
	case X86::MMX_PMULHWrr:
	case X86::MMX_PMULLWrm:
	case X86::MMX_PMULLWrr:
	case X86::MMX_PMULUDQrm:
	case X86::MMX_PMULUDQrr:
	case X86::MMX_PORrm:
	case X86::MMX_PORrr:
	case X86::MMX_PSADBWrm:
	case X86::MMX_PSADBWrr:
	case X86::MMX_PSHUFWmi:
	case X86::MMX_PSHUFWri:
	case X86::MMX_PSLLDri:
	case X86::MMX_PSLLDrm:
	case X86::MMX_PSLLDrr:
	case X86::MMX_PSLLQri:
	case X86::MMX_PSLLQrm:
	case X86::MMX_PSLLQrr:
	case X86::MMX_PSLLWri:
	case X86::MMX_PSLLWrm:
	case X86::MMX_PSLLWrr:
	case X86::MMX_PSRADri:
	case X86::MMX_PSRADrm:
	case X86::MMX_PSRADrr:
	case X86::MMX_PSRAWri:
	case X86::MMX_PSRAWrm:
	case X86::MMX_PSRAWrr:
	case X86::MMX_PSRLDri:
	case X86::MMX_PSRLDrm:
	case X86::MMX_PSRLDrr:
	case X86::MMX_PSRLQri:
	case X86::MMX_PSRLQrm:
	case X86::MMX_PSRLQrr:
	case X86::MMX_PSRLWri:
	case X86::MMX_PSRLWrm:
	case X86::MMX_PSRLWrr:
	case X86::MMX_PSUBBrm:
	case X86::MMX_PSUBBrr:
	case X86::MMX_PSUBDrm:
	case X86::MMX_PSUBDrr:
	case X86::MMX_PSUBQrm:
	case X86::MMX_PSUBQrr:
	case X86::MMX_PSUBSBrm:
	case X86::MMX_PSUBSBrr:
	case X86::MMX_PSUBSWrm:
	case X86::MMX_PSUBSWrr:
	case X86::MMX_PSUBUSBrm:
	case X86::MMX_PSUBUSBrr:
	case X86::MMX_PSUBUSWrm:
	case X86::MMX_PSUBUSWrr:
	case X86::MMX_PSUBWrm:
	case X86::MMX_PSUBWrr:
	case X86::MMX_PUNPCKHBWrm:
	case X86::MMX_PUNPCKHBWrr:
	case X86::MMX_PUNPCKHDQrm:
	case X86::MMX_PUNPCKHDQrr:
	case X86::MMX_PUNPCKHWDrm:
	case X86::MMX_PUNPCKHWDrr:
	case X86::MMX_PUNPCKLBWrm:
	case X86::MMX_PUNPCKLBWrr:
	case X86::MMX_PUNPCKLDQrm:
	case X86::MMX_PUNPCKLDQrr:
	case X86::MMX_PUNPCKLWDrm:
	case X86::MMX_PUNPCKLWDrr:
	case X86::MMX_PXORrm:
	case X86::MMX_PXORrr:
	case X86::MMX_V_SET0:
	case X86::MMX_V_SETALLONES:
		return turnOnFUT(4, FUT_MMX, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;
	
	// Vector ALU stuff
	case X86::HADDPDrm:
	case X86::HADDPDrr:
	case X86::HADDPSrm:
	case X86::HADDPSrr:
	case X86::HSUBPDrm:
	case X86::HSUBPDrr:
	case X86::HSUBPSrm:
	case X86::HSUBPSrr:
	case X86::PHADDDrm128:
	case X86::PHADDDrm64:
	case X86::PHADDDrr128:
	case X86::PHADDDrr64:
	case X86::PHADDSWrm128:
	case X86::PHADDSWrm64:
	case X86::PHADDSWrr128:
	case X86::PHADDSWrr64:
	case X86::PHADDWrm128:
	case X86::PHADDWrm64:
	case X86::PHADDWrr128:
	case X86::PHADDWrr64:
	case X86::PADDBrm:
	case X86::PADDBrr:
	case X86::PADDDrm:
	case X86::PADDDrr:
	case X86::PADDQrm:
	case X86::PADDQrr:
	case X86::PADDSBrm:
	case X86::PADDSBrr:
	case X86::PADDSWrm:
	case X86::PADDSWrr:
	case X86::PADDUSBrm:
	case X86::PADDUSBrr:
	case X86::PADDUSWrm:
	case X86::PADDUSWrr:
	case X86::PADDWrm:
	case X86::PADDWrr:
	case X86::PANDNrm:
	case X86::PANDNrr:
	case X86::PANDrm:
	case X86::PANDrr:
	case X86::PHSUBDrm128:
	case X86::PHSUBDrm64:
	case X86::PHSUBDrr128:
	case X86::PHSUBDrr64:
	case X86::PHSUBSWrm128:
	case X86::PHSUBSWrm64:
	case X86::PHSUBSWrr128:
	case X86::PHSUBSWrr64:
	case X86::PHSUBWrm128:
	case X86::PHSUBWrm64:
	case X86::PHSUBWrr128:
	case X86::PHSUBWrr64:
	case X86::PSUBBrm:
	case X86::PSUBBrr:
	case X86::PSUBDrm:
	case X86::PSUBDrr:
	case X86::PSUBQrm:
	case X86::PSUBQrr:
	case X86::PSUBSBrm:
	case X86::PSUBSBrr:
	case X86::PSUBSWrm:
	case X86::PSUBSWrr:
	case X86::PSUBUSBrm:
	case X86::PSUBUSBrr:
	case X86::PSUBUSWrm:
	case X86::PSUBUSWrr:
	case X86::PSUBWrm:
	case X86::PSUBWrr:
	case X86::PXORrm:
	case X86::PXORrr:
		return turnOnFUT(4, FUT_VECTOR_ALU, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;
	
	// Vector Multiply
	case X86::PMULDQrm:
	case X86::PMULDQrr:
	case X86::PMULHRSWrm128:
	case X86::PMULHRSWrm64:
	case X86::PMULHRSWrr128:
	case X86::PMULHRSWrr64:
	case X86::PMULHUWrm:
	case X86::PMULHUWrr:
	case X86::PMULHWrm:
	case X86::PMULHWrr:
	case X86::PMULLDrm:
	case X86::PMULLDrm_int:
	case X86::PMULLDrr:
	case X86::PMULLDrr_int:
	case X86::PMULLWrm:
	case X86::PMULLWrr:
	case X86::PMULUDQrm:
	case X86::PMULUDQrr:
		return turnOnFUT(4, FUT_VECTOR_MUL, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// General Vector Stuff
	case X86::PABSBrm128:
	case X86::PABSBrm64:
	case X86::PABSBrr128:
	case X86::PABSBrr64:
	case X86::PABSDrm128:
	case X86::PABSDrm64:
	case X86::PABSDrr128:
	case X86::PABSDrr64:
	case X86::PABSWrm128:
	case X86::PABSWrm64:
	case X86::PABSWrr128:
	case X86::PABSWrr64:
	case X86::PACKSSDWrm:
	case X86::PACKSSDWrr:
	case X86::PACKSSWBrm:
	case X86::PACKSSWBrr:
	case X86::PACKUSDWrm:
	case X86::PACKUSDWrr:
	case X86::PACKUSWBrm:
	case X86::PACKUSWBrr:
	case X86::PALIGNR128rm:
	case X86::PALIGNR128rr:
	case X86::PALIGNR64rm:
	case X86::PALIGNR64rr:
	case X86::PAVGBrm:
	case X86::PAVGBrr:
	case X86::PAVGWrm:
	case X86::PAVGWrr:
	case X86::PBLENDVBrm0:
	case X86::PBLENDVBrr0:
	case X86::PBLENDWrmi:
	case X86::PBLENDWrri:
	case X86::PCMPEQBrm:
	case X86::PCMPEQBrr:
	case X86::PCMPEQDrm:
	case X86::PCMPEQDrr:
	case X86::PCMPEQQrm:
	case X86::PCMPEQQrr:
	case X86::PCMPEQWrm:
	case X86::PCMPEQWrr:
	case X86::PCMPGTBrm:
	case X86::PCMPGTBrr:
	case X86::PCMPGTDrm:
	case X86::PCMPGTDrr:
	case X86::PCMPGTWrm:
	case X86::PCMPGTWrr:
	case X86::PEXTRBmr:
	case X86::PEXTRBrr:
	case X86::PEXTRDmr:
	case X86::PEXTRDrr:
	case X86::PEXTRQmr:
	case X86::PEXTRQrr:
	case X86::PEXTRWmr:
	case X86::PEXTRWri:
	case X86::PHMINPOSUWrm128:
	case X86::PHMINPOSUWrr128:
	case X86::PINSRBrm:
	case X86::PINSRBrr:
	case X86::PINSRDrm:
	case X86::PINSRDrr:
	case X86::PINSRQrm:
	case X86::PINSRQrr:
	case X86::PINSRWrmi:
	case X86::PINSRWrri:
	case X86::PMADDUBSWrm128:
	case X86::PMADDUBSWrm64:
	case X86::PMADDUBSWrr128:
	case X86::PMADDUBSWrr64:
	case X86::PMADDWDrm:
	case X86::PMADDWDrr:
	case X86::PMAXSBrm:
	case X86::PMAXSBrr:
	case X86::PMAXSDrm:
	case X86::PMAXSDrr:
	case X86::PMAXSWrm:
	case X86::PMAXSWrr:
	case X86::PMAXUBrm:
	case X86::PMAXUBrr:
	case X86::PMAXUDrm:
	case X86::PMAXUDrr:
	case X86::PMAXUWrm:
	case X86::PMAXUWrr:
	case X86::PMINSBrm:
	case X86::PMINSBrr:
	case X86::PMINSDrm:
	case X86::PMINSDrr:
	case X86::PMINSWrm:
	case X86::PMINSWrr:
	case X86::PMINUBrm:
	case X86::PMINUBrr:
	case X86::PMINUDrm:
	case X86::PMINUDrr:
	case X86::PMINUWrm:
	case X86::PMINUWrr:
	case X86::PMOVMSKBrr:
	case X86::PMOVSXBDrm:
	case X86::PMOVSXBDrr:
	case X86::PMOVSXBQrm:
	case X86::PMOVSXBQrr:
	case X86::PMOVSXBWrm:
	case X86::PMOVSXBWrr:
	case X86::PMOVSXDQrm:
	case X86::PMOVSXDQrr:
	case X86::PMOVSXWDrm:
	case X86::PMOVSXWDrr:
	case X86::PMOVSXWQrm:
	case X86::PMOVSXWQrr:
	case X86::PMOVZXBDrm:
	case X86::PMOVZXBDrr:
	case X86::PMOVZXBQrm:
	case X86::PMOVZXBQrr:
	case X86::PMOVZXBWrm:
	case X86::PMOVZXBWrr:
	case X86::PMOVZXDQrm:
	case X86::PMOVZXDQrr:
	case X86::PMOVZXWDrm:
	case X86::PMOVZXWDrr:
	case X86::PMOVZXWQrm:
	case X86::PMOVZXWQrr:
	case X86::PSHUFBrm128:
	case X86::PSHUFBrm64:
	case X86::PSHUFBrr128:
	case X86::PSHUFBrr64:
	case X86::PSHUFDmi:
	case X86::PSHUFDri:
	case X86::PSHUFHWmi:
	case X86::PSHUFHWri:
	case X86::PSHUFLWmi:
	case X86::PSHUFLWri:
	case X86::PSIGNBrm128:
	case X86::PSIGNBrm64:
	case X86::PSIGNBrr128:
	case X86::PSIGNBrr64:
	case X86::PSIGNDrm128:
	case X86::PSIGNDrm64:
	case X86::PSIGNDrr128:
	case X86::PSIGNDrr64:
	case X86::PSIGNWrm128:
	case X86::PSIGNWrm64:
	case X86::PSIGNWrr128:
	case X86::PSIGNWrr64:
	case X86::PSLLDQri:
	case X86::PSLLDri:
	case X86::PSLLDrm:
	case X86::PSLLDrr:
	case X86::PSLLQri:
	case X86::PSLLQrm:
	case X86::PSLLQrr:
	case X86::PSLLWri:
	case X86::PSLLWrm:
	case X86::PSLLWrr:
	case X86::PSRADri:
	case X86::PSRADrm:
	case X86::PSRADrr:
	case X86::PSRAWri:
	case X86::PSRAWrm:
	case X86::PSRAWrr:
	case X86::PSRLDQri:
	case X86::PSRLDri:
	case X86::PSRLDrm:
	case X86::PSRLDrr:
	case X86::PSRLQri:
	case X86::PSRLQrm:
	case X86::PSRLQrr:
	case X86::PSRLWri:
	case X86::PSRLWrm:
	case X86::PSRLWrr:
	case X86::PTESTrm:
	case X86::PTESTrr:
		return turnOnFUT(4, FUT_VECTOR, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Shift ops
	case X86::SAR16m1:
	case X86::SAR16mCL:
	case X86::SAR16mi:
	case X86::SAR16r1:
	case X86::SAR16rCL:
	case X86::SAR16ri:
	case X86::SAR32m1:
	case X86::SAR32mCL:
	case X86::SAR32mi:
	case X86::SAR32r1:
	case X86::SAR32rCL:
	case X86::SAR32ri:
	case X86::SAR64m1:
	case X86::SAR64mCL:
	case X86::SAR64mi:
	case X86::SAR64r1:
	case X86::SAR64rCL:
	case X86::SAR64ri:
	case X86::SAR8m1:
	case X86::SAR8mCL:
	case X86::SAR8mi:
	case X86::SAR8r1:
	case X86::SAR8rCL:
	case X86::SAR8ri:
	case X86::SHL16m1:
	case X86::SHL16mCL:
	case X86::SHL16mi:
	case X86::SHL16rCL:
	case X86::SHL16ri:
	case X86::SHL32m1:
	case X86::SHL32mCL:
	case X86::SHL32mi:
	case X86::SHL32rCL:
	case X86::SHL32ri:
	case X86::SHL64m1:
	case X86::SHL64mCL:
	case X86::SHL64mi:
	case X86::SHL64rCL:
	case X86::SHL64ri:
	case X86::SHL8m1:
	case X86::SHL8mCL:
	case X86::SHL8mi:
	case X86::SHL8rCL:
	case X86::SHL8ri:
	case X86::SHLD16mrCL:
	case X86::SHLD16mri8:
	case X86::SHLD16rrCL:
	case X86::SHLD16rri8:
	case X86::SHLD32mrCL:
	case X86::SHLD32mri8:
	case X86::SHLD32rrCL:
	case X86::SHLD32rri8:
	case X86::SHLD64mrCL:
	case X86::SHLD64mri8:
	case X86::SHLD64rrCL:
	case X86::SHLD64rri8:
	case X86::SHR16m1:
	case X86::SHR16mCL:
	case X86::SHR16mi:
	case X86::SHR16r1:
	case X86::SHR16rCL:
	case X86::SHR16ri:
	case X86::SHR32m1:
	case X86::SHR32mCL:
	case X86::SHR32mi:
	case X86::SHR32r1:
	case X86::SHR32rCL:
	case X86::SHR32ri:
	case X86::SHR64m1:
	case X86::SHR64mCL:
	case X86::SHR64mi:
	case X86::SHR64r1:
	case X86::SHR64rCL:
	case X86::SHR64ri:
	case X86::SHR8m1:
	case X86::SHR8mCL:
	case X86::SHR8mi:
	case X86::SHR8r1:
	case X86::SHR8rCL:
	case X86::SHR8ri:
	case X86::SHRD16mrCL:
	case X86::SHRD16mri8:
	case X86::SHRD16rrCL:
	case X86::SHRD16rri8:
	case X86::SHRD32mrCL:
	case X86::SHRD32mri8:
	case X86::SHRD32rrCL:
	case X86::SHRD32rri8:
	case X86::SHRD64mrCL:
	case X86::SHRD64mri8:
	case X86::SHRD64rrCL:
	case X86::SHRD64rri8:
	case X86::ROL16m1:
	case X86::ROL16mCL:
	case X86::ROL16mi:
	case X86::ROL16r1:
	case X86::ROL16rCL:
	case X86::ROL16ri:
	case X86::ROL32m1:
	case X86::ROL32mCL:
	case X86::ROL32mi:
	case X86::ROL32r1:
	case X86::ROL32rCL:
	case X86::ROL32ri:
	case X86::ROL64m1:
	case X86::ROL64mCL:
	case X86::ROL64mi:
	case X86::ROL64r1:
	case X86::ROL64rCL:
	case X86::ROL64ri:
	case X86::ROL8m1:
	case X86::ROL8mCL:
	case X86::ROL8mi:
	case X86::ROL8r1:
	case X86::ROL8rCL:
	case X86::ROL8ri:
	case X86::ROR16m1:
	case X86::ROR16mCL:
	case X86::ROR16mi:
	case X86::ROR16r1:
	case X86::ROR16rCL:
	case X86::ROR16ri:
	case X86::ROR32m1:
	case X86::ROR32mCL:
	case X86::ROR32mi:
	case X86::ROR32r1:
	case X86::ROR32rCL:
	case X86::ROR32ri:
	case X86::ROR64m1:
	case X86::ROR64mCL:
	case X86::ROR64mi:
	case X86::ROR64r1:
	case X86::ROR64rCL:
	case X86::ROR64ri:
	case X86::ROR8m1:
	case X86::ROR8mCL:
	case X86::ROR8mi:
	case X86::ROR8r1:
	case X86::ROR8rCL:
	case X86::ROR8ri:
		return turnOnFUT(4, FUT_LOAD, FUT_STORE, FUT_AGU, FUT_INT_SHIFT);
		break;
	
	// Complex Math ops
	case X86::SIN_F:
	case X86::SIN_Fp32:
	case X86::SIN_Fp64:
	case X86::SIN_Fp80:
	case X86::SQRTPDm:
	case X86::SQRTPDm_Int:
	case X86::SQRTPDr:
	case X86::SQRTPDr_Int:
	case X86::SQRTPSm:
	case X86::SQRTPSm_Int:
	case X86::SQRTPSr:
	case X86::SQRTPSr_Int:
	case X86::SQRTSDm:
	case X86::SQRTSDm_Int:
	case X86::SQRTSDr:
	case X86::SQRTSDr_Int:
	case X86::SQRTSSm:
	case X86::SQRTSSm_Int:
	case X86::SQRTSSr:
	case X86::SQRTSSr_Int:
	case X86::SQRT_F:
	case X86::SQRT_Fp32:
	case X86::SQRT_Fp64:
	case X86::SQRT_Fp80:
		return turnOnFUT(6, FUT_FP_SQRT, FUT_FP_MUL, FUT_FP_ADD, FUT_LOAD, FUT_STORE, FUT_AGU);
		break;

	// Unimplemented ops
	case X86::ABS_F:
	case X86::ABS_Fp32:
	case X86::ABS_Fp64:
	case X86::ABS_Fp80:
	case X86::ATOMAND32:
	case X86::ATOMMAX32:
	case X86::ATOMMIN32:
	case X86::ATOMOR32:
	case X86::ATOMUMAX32:
	case X86::ATOMUMIN32:
	case X86::ATOMXOR32:
	case X86::BLENDPDrmi:
	case X86::BLENDPDrri:
	case X86::BLENDPSrmi:
	case X86::BLENDPSrri:
	case X86::BLENDVPDrm0:
	case X86::BLENDVPDrr0:
	case X86::BLENDVPSrm0:
	case X86::BLENDVPSrr0:
	case X86::BSF16rm:
	case X86::BSF16rr:
	case X86::BSF32rm:
	case X86::BSF32rr:
	case X86::BSF64rm:
	case X86::BSF64rr:
	case X86::BSR16rm:
	case X86::BSR16rr:
	case X86::BSR32rm:
	case X86::BSR32rr:
	case X86::BSR64rm:
	case X86::BSR64rr:
	case X86::BSWAP32r:
	case X86::BSWAP64r:
	case X86::CBW:
	case X86::CDQ:
	case X86::CDQE:
	case X86::CHS_F:
	case X86::CHS_Fp32:
	case X86::CHS_Fp64:
	case X86::CHS_Fp80:
	case X86::CLFLUSH:
	case X86::COS_F:
	case X86::COS_Fp32:
	case X86::COS_Fp64:
	case X86::COS_Fp80:
	case X86::CQO:
	case X86::CWD:
	case X86::CWDE:
	case X86::DPPDrmi: case X86::DPPDrri: case X86::DPPSrmi: case X86::DPPSrri:
	case X86::DWARF_LOC:
	case X86::EH_RETURN:
	case X86::EXTRACTPSmr:
	case X86::EXTRACTPSrr:
	case X86::FNSTCW16m:
	case X86::FNSTSW8r:
	case X86::INSERTPSrm:
	case X86::INSERTPSrr:
	case X86::ISTT_FP16m:
	case X86::ISTT_FP32m:
	case X86::ISTT_FP64m:
	case X86::ISTT_Fp16m32:
	case X86::ISTT_Fp16m64:
	case X86::ISTT_Fp16m80:
	case X86::ISTT_Fp32m32:
	case X86::ISTT_Fp32m64:
	case X86::ISTT_Fp32m80:
	case X86::ISTT_Fp64m32:
	case X86::ISTT_Fp64m64:
	case X86::ISTT_Fp64m80:
	case X86::IST_F16m:
	case X86::IST_F32m:
	case X86::IST_FP16m:
	case X86::IST_FP32m:
	case X86::IST_FP64m:
	case X86::IST_Fp16m32:
	case X86::IST_Fp16m64:
	case X86::IST_Fp16m80:
	case X86::IST_Fp32m32:
	case X86::IST_Fp32m64:
	case X86::IST_Fp32m80:
	case X86::IST_Fp64m32:
	case X86::IST_Fp64m64:
	case X86::IST_Fp64m80:
	case X86::Int_CMPSDrm:
	case X86::Int_CMPSDrr:
	case X86::Int_CMPSSrm:
	case X86::Int_CMPSSrr:
	case X86::Int_COMISDrm:
	case X86::Int_COMISDrr:
	case X86::Int_COMISSrm:
	case X86::Int_COMISSrr:
	case X86::Int_UCOMISDrm:
	case X86::Int_UCOMISDrr:
	case X86::Int_UCOMISSrm:
	case X86::Int_UCOMISSrr:
	case X86::LAHF:
	case X86::LCMPXCHG16:
	case X86::LCMPXCHG32:
	case X86::LCMPXCHG64:
	case X86::LCMPXCHG8:
	case X86::LCMPXCHG8B:
	case X86::LEAVE:
	case X86::LEAVE64:
	case X86::LFENCE:
	case X86::LXADD16:
	case X86::LXADD32:
	case X86::LXADD64:
	case X86::LXADD8:
	case X86::MASKMOVDQU:
	case X86::MAXPDrm:
	case X86::MAXPDrm_Int:
	case X86::MAXPDrr:
	case X86::MAXPDrr_Int:
	case X86::MAXPSrm:
	case X86::MAXPSrm_Int:
	case X86::MAXPSrr:
	case X86::MAXPSrr_Int:
	case X86::MAXSDrm:
	case X86::MAXSDrm_Int:
	case X86::MAXSDrr:
	case X86::MAXSDrr_Int:
	case X86::MAXSSrm:
	case X86::MAXSSrm_Int:
	case X86::MAXSSrr:
	case X86::MAXSSrr_Int:
	case X86::MFENCE:
	case X86::MINPDrm:
	case X86::MINPDrm_Int:
	case X86::MINPDrr:
	case X86::MINPDrr_Int:
	case X86::MINPSrm:
	case X86::MINPSrm_Int:
	case X86::MINPSrr:
	case X86::MINPSrr_Int:
	case X86::MINSDrm:
	case X86::MINSDrm_Int:
	case X86::MINSDrr:
	case X86::MINSDrr_Int:
	case X86::MINSSrm:
	case X86::MINSSrm_Int:
	case X86::MINSSrr:
	case X86::MINSSrr_Int:
	case X86::MONITOR:
	case X86::MPSADBWrmi:
	case X86::MPSADBWrri:
	case X86::MWAIT:
	case X86::OUT16ir:
	case X86::OUT16rr:
	case X86::OUT32ir:
	case X86::OUT32rr:
	case X86::OUT8ir:
	case X86::OUT8rr:
	case X86::PORrm:
	case X86::PORrr:
	case X86::PREFETCHNTA:
	case X86::PREFETCHT0:
	case X86::PREFETCHT1:
	case X86::PREFETCHT2:
	case X86::PSADBWrm:
	case X86::PSADBWrr:
	case X86::PUNPCKHBWrm:
	case X86::PUNPCKHBWrr:
	case X86::PUNPCKHDQrm:
	case X86::PUNPCKHDQrr:
	case X86::PUNPCKHQDQrm:
	case X86::PUNPCKHQDQrr:
	case X86::PUNPCKHWDrm:
	case X86::PUNPCKHWDrr:
	case X86::PUNPCKLBWrm:
	case X86::PUNPCKLBWrr:
	case X86::PUNPCKLDQrm:
	case X86::PUNPCKLDQrr:
	case X86::PUNPCKLQDQrm:
	case X86::PUNPCKLQDQrr:
	case X86::PUNPCKLWDrm:
	case X86::PUNPCKLWDrr:
	case X86::RCPPSm:
	case X86::RCPPSm_Int:
	case X86::RCPPSr:
	case X86::RCPPSr_Int:
	case X86::RCPSSm:
	case X86::RCPSSm_Int:
	case X86::RCPSSr:
	case X86::RCPSSr_Int:
	case X86::REP_MOVSB:
	case X86::REP_MOVSD:
	case X86::REP_MOVSQ:
	case X86::REP_MOVSW:
	case X86::REP_STOSB:
	case X86::REP_STOSD:
	case X86::REP_STOSQ:
	case X86::REP_STOSW:
	case X86::ROUNDPDm_Int:
	case X86::ROUNDPDr_Int:
	case X86::ROUNDPSm_Int:
	case X86::ROUNDPSr_Int:
	case X86::ROUNDSDm_Int:
	case X86::ROUNDSDr_Int:
	case X86::ROUNDSSm_Int:
	case X86::ROUNDSSr_Int:
	case X86::RSQRTPSm:
	case X86::RSQRTPSm_Int:
	case X86::RSQRTPSr:
	case X86::RSQRTPSr_Int:
	case X86::RSQRTSSm:
	case X86::RSQRTSSm_Int:
	case X86::RSQRTSSr:
	case X86::RSQRTSSr_Int:
	case X86::SAHF:
	case X86::SBB32mi:
	case X86::SBB32mi8:
	case X86::SBB32mr:
	case X86::SBB32ri:
	case X86::SBB32ri8:
	case X86::SBB32rm:
	case X86::SBB32rr:
	case X86::SBB64mi32:
	case X86::SBB64mi8:
	case X86::SBB64mr:
	case X86::SBB64ri32:
	case X86::SBB64ri8:
	case X86::SBB64rm:
	case X86::SBB64rr:
	case X86::SBB8mi:
	case X86::SFENCE:
	case X86::SHUFPDrmi:
	case X86::SHUFPDrri:
	case X86::SHUFPSrmi:
	case X86::SHUFPSrri:
	case X86::STMXCSR:
	case X86::TCRETURNdi:
	case X86::TCRETURNdi64:
	case X86::TCRETURNri:
	case X86::TCRETURNri64:
	case X86::TLS_addr32:
	case X86::TLS_addr64:
	case X86::TLS_gs_ri:
	case X86::TLS_gs_rr:
	case X86::TLS_tp:
	case X86::TRAP:
	case X86::TST_F:
	case X86::TST_Fp32:
	case X86::TST_Fp64:
	case X86::TST_Fp80:
	case X86::UCOMISDrm:
	case X86::UCOMISDrr:
	case X86::UCOMISSrm:
	case X86::UCOMISSrr:
	case X86::UCOM_FIPr:
	case X86::UCOM_FIr:
	case X86::UCOM_FPPr:
	case X86::UCOM_FPr:
	case X86::UCOM_FpIr32:
	case X86::UCOM_FpIr64:
	case X86::UCOM_FpIr80:
	case X86::UCOM_Fpr32:
	case X86::UCOM_Fpr64:
	case X86::UCOM_Fpr80:
	case X86::UCOM_Fr:
	case X86::UNPCKHPDrm:
	case X86::UNPCKHPDrr:
	case X86::UNPCKHPSrm:
	case X86::UNPCKHPSrr:
	case X86::UNPCKLPDrm:
	case X86::UNPCKLPDrr:
	case X86::UNPCKLPSrm:
	case X86::UNPCKLPSrr:
	case X86::V_SET0:
	case X86::V_SETALLONES:
	case X86::XCHG16rm:
	case X86::XCHG32rm:
	case X86::XCHG64rm:
	case X86::XCHG8rm:
	case X86::XCH_F:
	case X86::INSTRUCTION_LIST_END:
	default:
		return turnOnFUT(FUT_ALL);
		break;
