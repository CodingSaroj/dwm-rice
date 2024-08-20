export XDG_DESKTOP=dwm
gsd-xsettings
sxhkd -c ~/.sxhkdrc &
$HOME/dwm-rice/wpswitcher &
picom --vsync --backend glx &
exec "$HOME/dwm-rice/rundwm"
