CID=$(docker create packages)
mkdir -p build-Release
docker cp $CID:/parser/build-Release/packages build-Release
docker cp $CID:/parser/build-Release/_site build-Release
docker rm $CID
