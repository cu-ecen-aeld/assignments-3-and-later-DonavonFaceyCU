#check for 2 args
if [ $# -ne 2 ]
then
    echo "Error. Needs two arguments"
    exit 1
fi

WRITE_FILE=$1
WRITE_STRING=$2

if [ -z "$WRITE_STRING" ]
then
    echo "Please enter a valid string to write"
    exit 1
fi

#make directory file will go in
WRITE_DIRECTORY="$(dirname "$WRITE_FILE")"
mkdir "$WRITE_DIRECTORY" -p

#check if directory exists
if ! [ -d "$WRITE_DIRECTORY" ]
then
    echo "Error. Directory failed to be created"
    exit 1
fi

touch $WRITE_FILE

#check if file exists
if ! [ -e "$WRITE_FILE" ]
then
    echo "Error. File failed to be created"
    exit 1
fi

#write string to file
echo $WRITE_STRING > $WRITE_FILE

exit