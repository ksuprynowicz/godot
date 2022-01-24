VERSION=1.5.0
rm -rf AUTHORS inc LICENSE src *.zip
curl -L -O https://github.com/google/draco/archive/refs/tags/$VERSION.zip
bsdtar --strip-components=1 -xvf *.zip
rm *.zip
rm -rf .github cmake pc docs unity testdata javascript maya CMakeLists.txt third_party/googletest
