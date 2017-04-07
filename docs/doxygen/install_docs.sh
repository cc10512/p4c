#! /bin/bash

set -e

topdir=`dirname $0`/../..
echo $topdir
cd $topdir

# record the current branch of the repo
current_branch=$(git status --porcelain --branch | head -1 | cut -f 2 -d " " | sed -e "s/\.\.\..*$//")
now=`date`
# generate the documentation
./bootstrap.sh --enable-doxygen-doc
cd build && make docs
cd ..
git fetch origin gh-pages
git checkout gh-pages
git pull --ff origin gh-pages
for ftype in html js md5 png map css ; do
    mv build/doxygen-out/html/*.$ftype ./
done
cp -r build/doxygen-out/html/search ./
git add *.html *.js *.map *.md5 *.png *.css search
git commit -m "updated docs for $now"
git push origin gh-pages
git checkout $current_branch
