#
# Build x16 release packages
#

# Note: Intel Mac needs
#   sudo ln -s /usr/local /opt/homebrew

set -e 

##############################
export PRERELEASE_VERSION=40
PRE=_pre1
##############################


PATCHES="`pwd`"

cd /tmp

rm -rf sdl2-win32
cp -prv ~/tmp/sdl2-win32 sdl2-win32

rm -rf x16-release
mkdir x16-release
cd x16-release

git clone https://github.com/commanderx16/x16-emulator
git clone https://github.com/commanderx16/x16-rom
git clone https://github.com/commanderx16/x16-docs
(cd x16-emulator; git reset --hard r$PRERELEASE_VERSION)
(cd x16-rom; git reset --hard r$PRERELEASE_VERSION)
(cd x16-docs; git reset --hard r$PRERELEASE_VERSION)
zip -r1m x16.zip x16-emulator x16-rom x16-docs

# Mac ARM

unzip x16.zip
(
        cd x16-emulator
        make package_mac
        mv x16emu_mac.zip ../x16emu_mac_arm.zip
)
rm -rf x16-emulator x16-rom x16-docs

# Mac Intel

scp x16.zip intel16.local:/tmp
ssh intel16.local "cd /tmp; rm -rf x16-emulator x16-rom x16-docs; unzip x16.zip; cd x16-emulator; PATH=/usr/local/bin:$PATH PRERELEASE_VERSION=$PRERELEASE_VERSION make package_mac"
scp intel16.local:/tmp/x16-emulator/x16emu_mac.zip x16emu_mac_intel.zip

# Mac Universal
(
        mkdir combine
        cd combine
        (mkdir arm; cd arm; unzip ../../x16emu_mac_arm.zip)
        (mkdir intel; cd intel; unzip ../../x16emu_mac_intel.zip)
        (
                cd arm
                mv x16emu x16emu.arm
                lipo x16emu.arm ../intel/x16emu -create -output x16emu
                zip -r ../../x16emu_mac.zip *
        )
)
rm -rf combine
rm x16emu_mac_arm.zip
rm x16emu_mac_intel.zip

# Linux

unzip x16.zip
(
        cd x16-emulator
        make package_linux
        mv x16emu_linux.zip ..
)
rm -rf x16-emulator x16-rom x16-docs

# Windows

unzip x16.zip
(
        cd x16-emulator
        patch -p1 < $PATCHES/patch1.diff
        make package_win
        mv x16emu_win.zip ..
)
rm -rf x16-emulator x16-rom x16-docs


# rename

for i in x16emu_mac x16emu_linux x16emu_win; do
        mv $i.zip $i-r$PRERELEASE_VERSION$PRE.zip
done

open .