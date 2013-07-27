#
#   we should almost certainly git clone ?? - so that if copy the tgz we can 
#  reimport later	
#
set -x


NAME=$( basename $( pwd ))

if [ -d "./tmp" ]; then
	echo "dir exists"
else 
	echo "dir does not exist (creating)"
	mkdir ./tmp
fi 

# must remove the internal .tgz before we tar it - otherwise it just expands !!
rm "./tmp/$NAME.tgz" 2> /dev/null
#ls ./tmp

tar czf "./tmp/$NAME.tgz" "../$NAME" --exclude 'external/*' --exclude 'build/*' --exclude 'bin/*' --exclude 'tmp/*' 

# --exclude 'data/*' 


# move to local tmp directory
#mv "/tmp/$NAME.tgz"  ./tmp 
ls -lh "./tmp/$NAME.tgz"

