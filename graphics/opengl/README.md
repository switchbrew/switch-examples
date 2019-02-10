# OpenGL Examples

## Necessary packages

These examples use OpenGL to render graphics with the GPU. In order to compile them, you need to install the necessary packages with the following command:

```bash
pacman -S switch-mesa switch-glad switch-glm
```

Note that on some systems, you may need to use `dkp-pacman` instead, and you may need to prefix the installation commands with `sudo`.

## Usage

It is not possible to use the libnx console and the GPU at the same time. For this reason, debugging output must be redirected to nxlink. All examples contain code that sets up stdout to redirect to the nxlink socket, however by default it's disabled. In order to enable it, you can `#define ENABLE_NXLINK` at the top of the file, or alternatively modify the Makefile to add `-DENABLE_NXLINK` to the `CFLAGS` variable.

Additionally, mesa and nouveau are presently configured to support debugging output. Each example has a `setMesaConfig` function that controls debugging and shader optimization flags. Please refer to the source code of this function for more details.
