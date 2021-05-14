# WSM2021
WSM2021 Homework


## Compile
g++ pugixml.cpp main.cpp -o main -O2
g++ pugixml.cpp sock.cpp -o sock -O2


## Config

Download some xml from (https://dumps.wikimedia.org/enwiki/latest/)[here]
Add it to the xml\_list

## Run

### Console

```
./main
```

Input some words, end of 'Q'

e.g.

```
./main
love Q
Return xxx pages in yyy sec
```

### Website


```
./sock
```

Open http://localhost:8080



## Note
works on linux only
