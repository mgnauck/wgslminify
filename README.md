# wgslminify

Utility to minify and mangle [WGSL](https://www.w3.org/TR/WGSL/), [WebGPU](https://www.w3.org/TR/webgpu/) shader code. When applying wgslminify, the shader code is tokenized and then several minification and mangling operations are applied. Output should be functionally equivalent to the original shader but smaller in text size.

## Usage

Input is read from stdin or file. The resulting shader code is printed to stdout. The following options are available:

* `-h` or `--help`: displays the command line help
* `-e`: will exclude the identifiers given in the comma separated list from mangling
* `--no-mangle`: will completely skip the mangling process

Examples:

```
$ wgslminify -e vertexMain,fragmentMain,myVariable fullshader.wgsl

$ wgslminify --no-mangle computeshader.wgsl

$ cat fragment.wgsl | wgslminify --no-mangle >fragment_out.wgsl
```

## Notes

Minification removes all kinds of comments, leading and trailing zeros of non-hexadecimal numeric literals (float and integer) and any unnecessary whitespaces.
Mangling replaces all identifiers with short character sequences. Identifiers with most occurrences will receive the shortest sequences.
Although not part of WGSL, JavaScript template literals (`${...}`) are detected and ignored during minification.

## Limitations

* Only ASCII shader code is supported. (WGSL is as per default UTF-8.)
* Unreachable (dead) code will not be removed. Unnecessary tokens (e.g. `((1.0 + ((2.0 * val))))`) won't be removed either.
* Identifiers named like swizzle names (xyzw/rgba, including any combination of these) are not replaced currently. I.e. if there is a `struct` with a member named `xxz`, this member won't be mangled. This might be improved sometime soon.
* Floating point numbers are currently not detected (matched) via regex, thus detection will fail in certain scenarios. A clear separation of numbers and operators by spaces (e.g. `1.0-ef` vs. `1.0 - ef`) avoids possible errors for now. This one is a strong candidate for improvement once time is available.
* Identifier mangling currently does not consider scope. This should be improved to yield better (smaller) mangling results. This is high on the list of improvements as well.
* Other reductions are certainly possible, e.g. converting `vec3<f32>(1.0, 1.0, 1.0)` to `vec3f(1)`.
* Likely there are other shortcomings and/or bugs. Testing was done only on a limited amout of shaders. Please let me know if you find an issue!
