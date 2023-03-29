# HTTP Server 

EPITA - Week6 - Programming

![Workflow](https://github.com/ProtossDragoon/epita-w6-prog-http-server/actions/workflows/master.yaml/badge.svg)

## Quickstart

`terminal`
```bash
$ git clone https://github.com/ProtossDragoon/epita-w6-prog-http-server.git
$ cd epita-w6-prog-http-server
```

### web_server

```bash
$ cd web_server && make
```

#### print_request

```bash
$ ./print_request
Static Server
Listening to port 2048...
GET / HTTP/1.1
Host: localhost:2048
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) Gecko/20100101 Firefox/73.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
Connection: keep-alive
Upgrade-Insecure-Requests: 1

GET /favicon.ico HTTP/1.1
Host: localhost:2048
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) Gecko/20100101 Firefox/73.0
Accept: image/webp,*/*
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
Connection: keep-alive
```

#### single_threaded

```bash
$ ./single_threaded
Static Server
Listening to port 2048...
4: index.html
4: style.css
4: image/bullet.png
4: hello_world.html
4: style.css
4: index.html
4: style.css
4: image/bullet.png

[...]

```

#### multithreaded

```bash
$ ./multithreaded
Static Server
Listening to port 2048...
4: index.html
5: style.css
4: image/bullet.png
5: slow.html
5: Sleeping for 10 seconds...
4: hello_world.html
6: style.css
4: style.css

[...]

```

### ttt_server

demo video

```bash
$ cd ttt_server && make && ./ttt_server
```

## Notes

If you are using VSCode, change your "type" in your launch.json file, e.g. `"type": "gdb"`
