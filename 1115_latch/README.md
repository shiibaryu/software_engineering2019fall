## Running examples

siiba@DESKTOP-1OT1PA5:/mnt/c/Users/ryuse/softwareEngineering2019/1115_latch$ time ./counter_non_lock<br>
Counter: 12103287 (Ref. 100000000)<br>

real    0m0.667s<br>
user    0m4.953s<br>
sys     0m0.016s<br>

siiba@DESKTOP-1OT1PA5:/mnt/c/Users/ryuse/softwareEngineering2019/1115_latch$ time ./counter_cas_lock<br>
Counter: 100000000 (Ref. 100000000)<br>

real    0m12.460s<br>
user    0m17.141s<br>
sys     0m23.750s<br>

siiba@DESKTOP-1OT1PA5:/mnt/c/Users/ryuse/softwareEngineering2019/1115_latch$ time ./counter_pthread_lock<br>
Counter: 100000000 (Ref. 100000000)<br>

real    0m4.702s<br>
user    0m4.781s<br>
sys     0m31.094s<br>

siiba@DESKTOP-1OT1PA5:/mnt/c/Users/ryuse/softwareEngineering2019/1115_latch$ time ./counter_fetch_and_add<br>
Counter: 100000000 (Ref. 100000000)<br>

real    0m1.544s<br>
user    0m11.922s<br>
sys     0m0.016s<br>
