# wgslminify

Utility to minify and mangle [WGSL](https://www.w3.org/TR/WGSL/), [WebGPU](https://www.w3.org/TR/webgpu/) shader code. When running wgslminify, your shader code is tokenized and then several minification and mangling operations are applied. Output should be functionally equivalent to the original shader but smaller in text size.

## Usage

Input is read from stdin or file. The resulting shader code is printed to stdout. The following options are available:

* `-h` or `--help`: displays the command line help
* `-e`: will exclude the identifiers given in the comma separated list from mangling
* `--no-mangle`: will completely skip the mangling process
* `--print-unused`: will not minify/mangle but print all function and variable identifiers that are unused and thus potentially redundant

Examples:

```
$ wgslminify -e vertexMain,fragmentMain,myVariable fullshader.wgsl

$ wgslminify --no-mangle computeshader.wgsl

$ cat fragment.wgsl | wgslminify --no-mangle >fragment_out.wgsl

$ wgslminify -e main --print-unused
```

## Notes

Minification removes all kinds of comments, leading and trailing zeros of non-hexadecimal numeric literals (float and integer) and unnecessary whitespaces.
Mangling replaces all identifiers with short character sequences. Identifiers with most occurrences will receive the shortest sequences.
Although not part of WGSL, JavaScript template literals (`${...}`) are detected and ignored during minification.

## Limitations (which might be rectified at some point in time)

* Only ASCII shader code was tested so far. (WGSL is as per default UTF-8.)
* Unused or unreachable code will not be removed. Unnecessary tokens (e.g. `((1.0 + ((2.0 * val))))`) won't be removed either.
* Identifiers named like swizzle names (xyzw/rgba, including any combination of these) are not replaced currently. I.e. if there is a `struct` with a member named `xxz`, this member won't be mangled.
* Floating point number matching might fail in some scenarios. A clear separation of numbers and operators by spaces (e.g. `1.0-ef` vs. `1.0 - ef`) avoids these errors for now.
* Identifier mangling currently does not consider scope. This could be improved to yield better (smaller) mangling results and is has a high priority for improvment.
* Other reductions are certainly possible, e.g. converting `vec3<f32>(1.0, 1.0, 1.0)` to `vec3f(1)`.
* Likely there are other shortcomings and/or bugs. Testing was done only on a limited amount of shaders. Please let me know if you find an issue!

## Build

Use the included makefile to build the project.
