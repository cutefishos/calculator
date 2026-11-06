# Calculator

The calculator of Cutefish Desktop: a simple keypad calculator for + − × ÷
and percentages (200 + 10% is 220), entered with the mouse or the keyboard,
with a short history of the calculations. It computes in decimal, so
0.1 + 0.2 is 0.3: whole numbers keep every digit up to 10^1000, everything
else 160 significant digits.

## Dependencies

Calculator uses cutefish-toolkit; GoogleTest for the tests.

```sh
sudo apt install cmake libgtest-dev
```

## Build and install

```sh
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

## Tests

```sh
cmake -DBUILD_TESTING=ON ..
make
ctest --output-on-failure
```

## License

BSD-3-Clause; see [LICENSE](LICENSE).
