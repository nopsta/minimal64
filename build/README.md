The build.bat script will create m64.js and m64.wasm.

m64.js can be added to a html document with a <script> tag. When it is loaded, it will attempt to load the binary file m64.wasm which should be located in the same directory as m64.js.

m64-singlefile.js contains both m64.js and a base64 encoded version of m64.wasm. It can be used without need for an extra binary file.

tl;dr: you need either:

- both m64.js and m64.wasm
or
- just m64-singlefile.js
