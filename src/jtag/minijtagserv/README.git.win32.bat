## This is a bat script to be run on CMD.exe prompt
##
## Git on windows have problem with Linux symlinks
## (1) "git status" always says the file has a "type change", and
##      directories are modified. 
## (2) You cannot do a "git pull" at all.
## (3) The symlinked directories are not updated
##
## Git Windows actually copy the files/directories.
##
## Workaround:
##  (a) convert to windows' own symlink
##  (b) tell git to ignore those file
##
## LIMITATAION
## (1) jtagservice.[hc] will not up tracked by git. 
##     jtagserivce.h is an issue

del jtagservice.c jtagservice.h lib64 win64
mklink jtagservice.c ../src/drivers/jtagserv/jtagservice.c
mklink jtagservice.h ../src/drivers/jtagserv/jtagservice.h
mklink lib64 ../src/drivers/jtagserv/lib64
mklink win64 ../src/drivers/jtagserv/win64
git update-index --assume-unchanged jtagservice.c jtagservice.h lib64 win64