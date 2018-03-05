#g++ -c ./*.cpp
#g++ -c ./compression/*.cpp
#g++ -c ./encription/*.cpp

#rm -r DataStructures
#mkdir DataStructures
#cp -r ./*.o DataStructures/

#mkdir -p DataStructures/lib
#ar rcs DataStructures/lib/libDataStructures.a DataStructures/*.o 
#rm -r DataStructures/*.o
#mkdir -p DataStructures/include/compression
#mkdir -p DataStructures/include/encription

#cp -r ./*.h DataStructures/include
#cp -r ./compression/*.h DataStructures/include/compression/
#cp -r ./encription/*.h DataStructures/include/encription/

g++ -c ./*.cpp
g++ -c ./compression/*.cpp
g++ -c ./encription/*.cpp
g++ -c ./singleFileSystem/*.cpp
g++ -c ./streams/*.cpp
g++ -c ./fileUtils/*.cpp
rm -r DataStructures
mkdir DataStructures
mkdir -p DataStructures/lib
ar rcs DataStructures/lib/libDataStructures.a *.o
rm -r *.o
mkdir -p DataStructures/include/compression
mkdir -p DataStructures/include/encription
mkdir -p DataStructures/include/fileUtils
mkdir -p DataStructures/include/singleFileSystem
mkdir -p DataStructures/include/streams 
cp -r ./*.h DataStructures/include
cp -r ./compression/*.h DataStructures/include/compression/
cp -r ./encription/*.h DataStructures/include/encription/
cp -r ./fileUtils/*.h  DataStructures/include/fileUtils
cp -r ./singleFileSystem/*.h DataStructures/include/singleFileSystem
cp -r ./streams/*.h DataStructures/include/streams
