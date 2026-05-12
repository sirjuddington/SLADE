# This script assumes SLADE (executable and pk3) has already been built and resides in ../dist/
# The linuxdeploy tool also needs to be installed and available in the system PATH

# Create AppDir
APPDIR=AppDir
rm -rf $APPDIR
mkdir -p $APPDIR/usr/share/metainfo
mkdir -p $APPDIR/usr/share/slade3

# Copy metainfo file
cp net.mancubus.SLADE.metainfo.xml $APPDIR/usr/share/metainfo/net.mancubus.SLADE.appdata.xml

# Copy pk3 if it exists in dist/, otherwise exit
if [ -f ../dist/slade.pk3 ]; then
    cp ../dist/slade.pk3 $APPDIR/usr/share/slade3/
else
    echo "Error: slade.pk3 not found in ../dist/"
    exit 1
fi

# Create AppImage
linuxdeploy --appdir AppDir --executable ../dist/slade --desktop-file net.mancubus.SLADE.desktop --icon-file net.mancubus.SLADE.svg --output appimage
