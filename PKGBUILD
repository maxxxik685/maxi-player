pkgname=maxi-player
pkgver=1.0.0
pkgrel=1
pkgdesc="Terminal music player"
arch=('x86_64')
url="https://github.com/maxxxik685/maxi-player"
license=('MIT')

depends=(
    'glibc'
    'mpg123'
    'libpulse'
)

makedepends=(
    'gcc'
    'make'
)

source=()

build() {
    cd "$srcdir"

    make -C "$startdir" clean
    make -C "$startdir"
}

package() {
    install -Dm755 "$startdir/maxi-player" \
        "$pkgdir/usr/bin/maxi-player"

    install -Dm644 "$startdir/LICENSE" \
        "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
