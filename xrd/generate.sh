gir -o .
rm -rf doc
gir -c Gir.toml -m doc -o doc
rustdoc-stripper -g -o doc/vendor.md
