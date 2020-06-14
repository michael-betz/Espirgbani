# extract all zip files
```bash
for f in *zip; do unar $f; done
```

# move all .ttf files into ./fnt
```bash
shopt -s globstar
for f in **/*.ttf; do mv "$f" ./fnt; done
```

# rename to 00.ttf, 01.ttf, etc
```bash
i=0; for f in *.ttf; do mv "$f" "$(printf %02d.ttf $i)"; i=$((i+1)); done
```
