#! /bin/bash
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Assemble Flatpak app files and metadata under /app/, copying from the
# installation tree generated by 'make distro-pack-install' (at
# $PREFIXDIR):

set -e

cp -r "${PREFIXDIR?}"/lib/libreoffice /app/
ln -s /app/libreoffice/program/soffice /app/bin/libreoffice

mkdir -p /app/share/applications
"${SRCDIR?}"/solenv/bin/assemble-flatpak-desktop.sh "${PREFIXDIR?}"/share/applications/ \
 /app/share/applications/

## icons/hicolor/*/apps/libreoffice-* ->
## icons/hicolor/*/apps/org.libreoffice.LibreOffice-*:
mkdir -p /app/share/icons
for i in "${PREFIXDIR?}"/share/icons/hicolor/*/apps/libreoffice-*
do
 mkdir -p \
  "$(dirname /app/share/icons/hicolor/"${i#"${PREFIXDIR?}"/share/icons/hicolor/}")"
 cp -a "$i" \
  "$(dirname /app/share/icons/hicolor/"${i#"${PREFIXDIR?}"/share/icons/hicolor/}")"/"$(basename "$i")"
 cp -a "$i" \
  "$(dirname /app/share/icons/hicolor/"${i#"${PREFIXDIR?}"/share/icons/hicolor/}")"/org.libreoffice.LibreOffice."${i##*/apps/libreoffice-}"
done

mkdir -p /app/share/runtime/locale
for i in $(ls /app/libreoffice/program/resource)
do
  lang="${i%[_@]*}"
  mkdir -p /app/share/runtime/locale/"${lang}"/resource
  mv /app/libreoffice/program/resource/"${i}" /app/share/runtime/locale/"${lang}"/resource
  ln -s ../../../share/runtime/locale/"${lang}"/resource/"${i}" /app/libreoffice/program/resource
done

for i in /app/libreoffice/share/registry/Langpack-*.xcd /app/libreoffice/share/registry/res/{fcfg_langpack,registry}_*.xcd
do
  basename="$(basename "${i}" .xcd)"
  lang="${basename#Langpack-}"
  lang="${lang#fcfg_langpack_}"
  lang="${lang#registry_}"

  # ship the base app with at least one Langpack/fcfg_langpack
  if [ "${lang}" = "en-US" ]
  then
    continue
  fi

  lang="${lang%-*}"
  mkdir -p /app/share/runtime/locale/"${lang}"/registry
  mv "${i}" /app/share/runtime/locale/"${lang}"/registry
  ln -rs /app/share/runtime/locale/"${lang}"/registry/"${basename}".xcd "${i}"
done

mkdir -p /app/share/appdata
"${SRCDIR?}"/solenv/bin/assemble-flatpak-appdata.sh /app/share/appdata/ 1

## see <https://github.com/flatpak/flatpak/blob/master/app/
## flatpak-builtins-build-finish.c> for further places where build-finish would
## look for data:
## cp ... /app/share/dbus-1/services/
## cp ... /app/share/gnome-shell/search-providers/
