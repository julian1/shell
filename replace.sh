


find src/ include/ -type f  | egrep '.*.cpp|.*.h' | while read i ; do echo $i; sed -i "s/$1/$2/g" $i ; done

