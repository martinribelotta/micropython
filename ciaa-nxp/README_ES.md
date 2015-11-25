Soporte para EDU-CIAA
========================
Table of context

[TOC]

<p align="center">
  <img src="https://avatars0.githubusercontent.com/u/6998305?v=3&s=128" alt="CIAA Logo"/>
</p>

### Modo de uso:

- Ejecutar `make clean all download` para compilar y bajar el programa en la EDU-CIAA.
- Ejecute una terminal serial a 115200 en el segundo puerton serial del USB (usualmente /dev/ttyUSB1 en Linux)
- Puede usar una linea de comandos python REPL en la UART del puerto USB de DEBUG.


## Soporte de hardware

### Modulo pyb:
##### Soporte de los 4 LEDS

Clase `pyb.LED`.

Ejemplo:
```python
import pyb
led1 = pyb.LED(1)
led1.on()
pyb.delay(100);
led1.off()
```
Numeros de leds disponibles: de 1 a 6 (4:Red, 5:Green, 6:Blue)
Mas info en : http://test-ergun.readthedocs.org/en/latest/library/pyb.LED.html

##### Soporte para los 4 pulsadores de la placa
Clase `pyb.Switch`.

Ejemplo:
```python
import pyb
switch1 = pyb.Switch(1)
val = switch1.value()
print('sw1 vale:'+str(val))
```
Numeros de switch disponibles:  de 1 a 4
Mas info en : http://test-ergun.readthedocs.org/en/latest/library/pyb.Switch.html

##### Soporte para la UART

Implementado RS232 del conector P1 y la UART RS485 de la placa mediante la clase `pyb.UART`.

Ejemplo:

```python
import pyb
uart = pyb.UART(3)
uart.init(115200,bits=8, parity=None, stop=1,timeout=1000, timeout_char=1000, read_buf_len=64)
uart.write("Hola mundo")
while True:
        if uart.any():
                print("hay data:")
                data = uart.readall()
                print(data)
```
Las UARTs disponibles son la "0" (RS485) y la "3" (RS232)

Mas info en : http://test-ergun.readthedocs.org/en/latest/library/pyb.UART.html
Se respeto la compatibilidad del modulo original de la pyboard exceptuando el constructor que recibe el baudrate.
Tambien se agrego un modo de recepcion por "paquete" en donde la trama que llega se almacena en un buffer interno y luego el metodo "any()" devuelve True.
Utilizando el metodo "readall()" se obtiene la trama completa. Para habilitar la recepcion por paquete se deben agregar el argumento "packet_mode" en True al final del metodo "init()". Ejemplo:
```python
uart.init(115200,bits=8, parity=None, stop=1,timeout=0, timeout_char=1000, read_buf_len=64,packet_mode=True)
```
Se interpretara como un "paquete" cuando comiencen a llegar bytes y se detenga la recepcion durante mas tiempo del tiempo especificado en el argumento "timeout"

Tambien es posible detectar el final del "paquete" mediante un caracter en particular en lugar de hacerlo por timeout, para ello se puede agregar dicho caracter como argumento en "init". Ejemplo:
```python
art.init(115200,bits=8, parity=None, stop=1,timeout=0, timeout_char=1000, read_buf_len=64,packet_mode=True,packet_end_char=ord('o'))
```
En este ejemplo, se detectara un fin de trama cuando llegue el caracter 'o'


##### Soporte para GPIOs

Clase `pyb.Pin`.

Ejemplo:
```python
import pyb

p = pyb.Pin(0) #GPIO0
p.init(pyb.Pin.OUT_PP,pyb.Pin.PULL_NONE)
print(p)

while True:
        p.high()
        print("value:"+str(p.value()))
        pyb.delay(1000)
        p.low()
        print("value:"+str(p.value()))
        pyb.delay(1000)
```
Las GPIOs disponibles van de 0 a 8

Mas info en : http://test-ergun.readthedocs.org/en/latest/library/pyb.Pin.html


##### Soporte para interrupciones en GPIOs
Clase `pyb.ExtInt`.

Ejemplo:
```python
import pyb

def callBack(line):
        print("Pin Interrupt!")
        print("Line = ",line)

p = pyb.Pin(8)
p.init(pyb.Pin.OUT_PP,pyb.Pin.PULL_NONE)
print(p)

int = pyb.ExtInt(p,pyb.ExtInt.IRQ_RISING,pyb.Pin.PULL_NONE,callBack)
print(int)

while True:
        pyb.delay(1000)
        print("tick")
```
Existen 4 interrupciones disponibles para asignar a cualquiera de las 9 GPIOs
Se implementaron los metodos:
- enable()
- disable()
- swint()
- line()

Mas info en : http://test-ergun.readthedocs.org/en/latest/library/pyb.ExtInt.html

##### Soporte para DAC
Clase `pyb.DAC`.

Ejemplo:
```python
import pyb
import math

dac = pyb.DAC(1)

# sine
buf = bytearray(200) #100 samples. 2 bytes per sample
j=0
for i in range (0,len(buf)/2):
        v = 512 + int(511 * math.sin(2 * math.pi * i / (len(buf)/2) ) )
        buf[j+1] = (v >>  8) & 0xff
        buf[j] = v & 0xff
        j=j+2

# output the sine-wave at 400Hz
print("sine created")

dac.write_timed(buf, 400*(int(len(buf)/2)), mode=pyb.DAC.CIRCULAR)

while True:
        pyb.delay(1000)

```
Existe solo el DAC 1

A diferencia de la clase DAC de la pyboard (http://test-ergun.readthedocs.org/en/latest/library/pyb.DAC.html) se utilizaron valores de 10bit en vez de 8bits para aprovechar al maximo la resolucion del DAC.


##### Soporte para Timers

Clase `pyb.Timer`.

Ejemplo:
```python
import pyb

def callb(timer):
        print("Interval interrupt")
        print(timer)

def callbTimeout (timer):
        print("Timeout interrupt")
        print(timer)

print("Test Timers")

t1 = pyb.Timer(1)
t2 = pyb.Timer(2)
t1.interval(2000,callb)
t2.timeout(5000,callbTimeout)

while True:
        pyb.delay(1000)

```
Los timers disponibles son el 0,1,2 y 3

Ademas de las funciones definidas en http://test-ergun.readthedocs.org/en/latest/library/pyb.Timer.html se agregaron los metodos interval y timeout. Ambos reciben
el tiempo en milisegundos y una funcion callback. El primero ejecutara la funcion cada el tiempo prefijado, mientras que el segundo la ejecutara solo una vez luego
del tiempo prefijado.

No se implemento la clase TimerChannel, por lo que las funcionalidades de Output Compare e Input Capture no son accesibles.


##### Soporte para PWM

Clase `pyb.PWM`

Ejemplo:
```python
import pyb

pyb.PWM.set_frequency(1000)

out0 = pyb.PWM(0)
out0.duty_cycle(50) # 50%
print("Duty cycle :"+str(out0.duty_cycle()))

out1= pyb.PWM(1)
out1.duty_cycle(25)

out10= pyb.PWM(10)
out10.duty_cycle(75)

while True:
        pyb.delay(1000)
```
Salidas de PWM disponibles: 0 a 10

- 0: GPIO_2
- 1: GPIO_8
- 2: T_FIL1
- 3: T_FIL2
- 4: T_FIL3
- 5: T_COL0
- 6: T_COL1
- 7: T_COL2
- 8: LCD_1
- 9: LCD_2
- 10: LCD_3

La placa posee un solo modulo PWM con 11 salidas asociadas, por esta razon todas las salidas comparten la misma frecuencia, pero tienen un valor de ciclo de actividad independiente.


##### Soporte para ADC

Clase `pyb.ADC`.

Ejemplo:
```python
import pyb

channel1 = pyb.ADC(1)
channel2 = pyb.ADC(2)
channel3 = pyb.ADC(3)

while True:
        v1 = channel1.read()
        v2 = channel2.read()
        v3 = channel3.read()
        print("value ch1:"+str(v1))
        print("value ch2:"+str(v2))
        print("value ch3:"+str(v3))
        pyb.delay(1000)
```
Entradas AD disponibles: 1,2 y 3. El resultado del metodo read es el valor de conversion (10 bit de resolucion en 3.3V)
