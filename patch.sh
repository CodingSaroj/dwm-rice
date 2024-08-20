cp -r dwm-vanilla dwm-stable
cd dwm-stable
for f in $(echo ../patches/dwm-*.diff); do patch < $f; done

cd ..
cp -r st-vanilla st-stable
cd st-stable
for f in $(echo ../patches/st-*.diff); do patch < $f; done

