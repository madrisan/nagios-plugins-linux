# Extra informations for developers

## Link to procps-ng newlib

### Get, configure, compile, and install procps-ng newlib

These instructions install a temporary copy of `procps-ng newlib` in the `/tmp` folder.
```
sudo dnf install gettext-devel

cd /var/tmp
git clone --single-branch --branch newlib https://gitlab.com/procps-ng/procps procps-ng-newlib
cd procps-ng-newlib/

PROCPSNG_VERSION="$(misc/git-version-gen .tarball-version)"

cat <<EOF >/tmp/libprocps-newlib.pc
prefix=/usr
exec_prefix=/
libdir=/tmp/procps-ng-newlib/usr/lib64
includedir=/tmp/procps-ng-newlib/usr/include

Name: libprocps
Description: Library to control and query process state
Version: $PROCPSNG_VERSION
Libs: -L\${libdir} -lprocps
Libs.private:
Cflags: -I\${includedir}
EOF

sudo mv /tmp/libprocps-newlib.pc \
        /usr/lib64/pkgconfig/libprocps.pc

autoreconf --install
./configure --prefix=/usr --without-ncurses \
    --disable-static --disable-pidof --disable-kill

# do not build the man related stuff
for d in po man-po; do
   echo "\
all:
install:" > $d/Makefile
done

make
make install DESTDIR=/tmp/procps-ng-newlib

rm -fr /tmp/procps-ng-newlib/usr/share
rm -fr /tmp/procps-ng-newlib/usr/sbin
rm -fr /tmp/procps-ng-newlib/usr/bin

sudo rm -f /usr/lib64/pkgconfig/libprocps.pc

find /tmp/procps-ng-newlib/ -type f
```

### Build `nagios-plugins-linux`

Build `nagios-plugins-linux` with `--enable-libprocps`.

```
git clone https://github.com/madrisan/nagios-plugins-linux.git
cd nagios-plugins-linux
./configure --enable-libprocps \
    CFLAGS="-O -I/tmp/procps-ng-newlib/usr/include" \
    LIBPROCPS_LIBS="-L/tmp/procps-ng-newlib/usr/lib64/ -lproc-2"
make
```

## Podman Plugin

Connect to systemd user service manager and start/enable podman socket:
```
systemctl --user start podman.socket
systemctl --user enable podman.socket
```
Check for API output:
```
curl --unix-socket /run/user/1000/podman/podman.sock http://d/v3.0.0/libpod/info | jq
curl --unix-socket /run/user/1000/podman/podman.sock -v 'http://d/v3.0.0/libpod/images/json' | jq
```

Podman API Reference: https://docs.podman.io/en/latest/Reference.html
