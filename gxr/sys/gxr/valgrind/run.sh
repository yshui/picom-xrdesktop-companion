#!/usr/bin/sh

export G_SLICE=always-malloc
export G_DEBUG=gc-friendly

SUPP_DIR=valgrind/suppressions

BIN_NAME=$(basename $1)

generate_suppressions()
{
  valgrind \
    -v \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --num-callers=20 \
    --show-reachable=yes \
    --error-limit=no \
    --track-origins=yes \
    --suppressions=$SUPP_DIR/external/GNOME.supp/base.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gdk.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/glib.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gtk3.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gio.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gobject.supp \
    --suppressions=$SUPP_DIR/external/glib.supp \
    --suppressions=$SUPP_DIR/mesa.supp \
    --suppressions=$SUPP_DIR/glib.supp \
    --suppressions=$SUPP_DIR/misc.supp \
    --suppressions=$SUPP_DIR/openvr.supp \
    --log-file=memcheck.log \
    --gen-suppressions=all \
    $1

  cat ./memcheck.log | valgrind/parse_suppressions.sh > $SUPP_DIR/$BIN_NAME.supp
}

show_memleaks()
{
  valgrind \
    -v \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --leak-resolution=high \
    --num-callers=20 \
    --show-reachable=yes \
    --error-limit=no \
    --track-origins=yes \
    --suppressions=$SUPP_DIR/external/GNOME.supp/base.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gdk.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/glib.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gtk3.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gio.supp \
    --suppressions=$SUPP_DIR/external/GNOME.supp/gobject.supp \
    --suppressions=$SUPP_DIR/external/glib.supp \
    --suppressions=$SUPP_DIR/mesa.supp \
    --suppressions=$SUPP_DIR/glib.supp \
    --suppressions=$SUPP_DIR/misc.supp \
    --suppressions=$SUPP_DIR/openvr.supp \
    $1
}

if [ -z ${1+x} ]; then
  echo "Usage: $0 <test_name>"
  exit 1;
fi

if [ -z ${2+x} ]; then
  show_memleaks $1
else
  case $2 in
      -g)
      generate_suppressions $1
      ;;
      *)    # unknown option
      echo "Unknown argument $2"
      ;;
  esac
fi




