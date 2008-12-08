rm cfg.main.dot
rm cfg.main.ps
opt -load ../../Debug/lib/profile_rt.so -profile-loader -print-cfg $1
dot -Tps -o cfg.main.ps cfg.main.dot
evince cfg.main.ps
