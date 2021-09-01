# Cypercine(WIP)

A benchmarker

### Building
```sh
make
```


### Installing
```sh
make install
```

[//]: # (### Configuring)

### Usage
```sh
./cypercine 1000 "ls -lAth"
```

```sh
./cypercine 10 "ls -lAG" "lx -la" "exa -l --icons" "gls -lA --color=auto" "lsd -lA" # bsd and gnu ls
```

```sh
./cypercine "ls" "lsd" "lx"
```
