


find src/ include/ -type f  | egrep '.*.cpp|.*.h' | while read i ; do grep -H $1 $i ; done

