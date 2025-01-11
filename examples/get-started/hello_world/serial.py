import serial

ser = serial.Serial(
    port='/dev/ttyUSB0',       # 串口号，Windows 上通常是 'COMx'，Linux 上通常是 '/dev/ttyUSBx' 或 '/dev/ttySx'
    baudrate=115200,     # 波特率
    parity=serial.PARITY_NONE,  # 校验位
    stopbits=serial.STOPBITS_ONE,  # 停止位
    bytesize=serial.EIGHTBITS,  # 数据位
    timeout=1          # 读取超时时间
)

if ser.is_open:
    print(f"串口 {ser.port} 已打开")

ser.write(b'Hello, Serial!')  # 发送字节数据

data = ser.readline()  # 读取一行数据
print(f"接收到的数据: {data.decode('utf-8')}")

ser.close()
if not ser.is_open:
    print(f"串口 {ser.port} 已关闭")
