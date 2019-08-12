# MKRS
MKRS controller diag

<ul>
<li><b>rc.local</b>	- GPIO preparing and runs "demo test"
<li><b>install</b>	- copies ./rc.local to /etc and symbol links ./build/demo to /usr/sbin
<li><b>demo test</b> 	- running leds
<li><b>demo adc</b> 	- reads all ADCs
<li><b>./inc/dp.h</b>	- headers file
<li><b>./src/dp.c</b>	- functions
<li><b>./src/main.c</b> - main C file
</ul>
<p>
<b>Compile:</b> in ./ folder run <b>make all</b></br/>
A new compiled file is ./build/demo. <br/>
For checking just run <b>demo</b>
</p>
