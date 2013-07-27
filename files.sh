# all source files

## can use like this
# for i in $(./files.sh) ; do echo $i; done
# for i in $(./files.sh) ; do sed -i 's/add_grid_edit_job/add/' $i; done

## or set a variable
# FILES=$(./files.sh);


echo $( find src test -iname '*.cpp'  ) $( find include/ -iname '*.h' )

 


