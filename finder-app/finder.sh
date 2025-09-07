#check for 2 args
if [ $# -ne 2 ]
then
    echo "Error. Needs two arguments"
    exit 1
fi

SEARCH_DIRECTORY=$1
SEARCH_STRING=$2

#check for valid filepath
if ! [ -d $SEARCH_DIRECTORY ]
then
    echo "Error. Invalid filepath"
    exit 1
fi

#use grep to count string matches in directory of interest.
LINE_COUNT=$( grep -r $SEARCH_STRING $SEARCH_DIRECTORY | wc -l )

#use find to count files in directory of interest.
FILE_COUNT=$( find $SEARCH_DIRECTORY -type f | wc -l )

echo "The number of files are $FILE_COUNT and the number of matching lines are $LINE_COUNT"
exit
