#/bin/bash
cp ./web/dist/index.html ./components/brew-engine/index.html
gzip -f ./components/brew-engine/index.html 