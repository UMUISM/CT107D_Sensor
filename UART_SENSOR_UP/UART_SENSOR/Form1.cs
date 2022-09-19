using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace UART_SENSOR
{
    public partial class Form1 : Form
    {
        private bool isStart = false;
        private bool device = false;
        private string rec_data = "";

        public Form1()
        {
            InitializeComponent();
            serialPort1.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);
            CheckForIllegalCrossThreadCalls = false;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // 自动搜索端口号并添加到列表中
            selectAndAddComport(serialPort1);
            button1.Enabled = false;
        }

        private void selectAndAddComport(SerialPort _SerialPort)
        {
            string selectCom = comboBox1.Text;
            comboBox1.Items.Clear();
            for (int i = 1; i <= 30; i++)
            {
                selectCom = "COM" + i.ToString();
                _SerialPort.PortName = selectCom;
                try
                {
                    _SerialPort.Open();
                    comboBox1.Items.Add(selectCom);
                    _SerialPort.Close();
                }
                catch { }
            }
            try
            {
                comboBox1.Text = comboBox1.Items[0].ToString();
            }
            catch { }
        }

        private void sendSerialData(string data)
        {
            if (serialPort1.IsOpen)
            {
                data = data.Replace("0x", string.Empty);
                Logger("Send -> " + data);

                string str = data;
                str = str.Replace(" ", string.Empty);

                byte[] _Bytes = new byte[str.Length / 2];

                for (int i = 0; i < str.Length / 2; i++)
                {
                    _Bytes[i] = Convert.ToByte(str.Substring(i * 2, 2), 16);
                }
                try
                {
                    serialPort1.Write(_Bytes, 0, str.Length / 2);
                }
                catch (Exception ex) // 串口断开连接
                {
                    serialPort1.Close();
                    button4.Enabled = true;

                    button1.Text = "开始监测";
                    button1.Enabled = false;

                    timer1.Enabled = false;
                    isStart = false;
                    MessageBox.Show("串口断开连接, " + ex.Message.ToString(), "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void Logger(string log)
        {
            textBox3.AppendText(DateTime.Now.ToString("hh:mm:ss") + ":  " + log + Environment.NewLine);
            textBox3.ScrollToCaret();
        }

        private void port_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            if (serialPort1.BytesToRead == 8)
            {
                string str = "";
                rec_data = "";
                byte[] revData = new byte[serialPort1.BytesToRead];
                serialPort1.Read(revData, 0, revData.Length);

                // LOG 数据
                foreach (byte i in revData)
                {
                    str = i.ToString("X").ToUpper();
                    rec_data += (str.Length == 2 ? str : "0" + str) + " ";
                }

                if (revData[0] != command.REC_START)
                {
                    Logger("返回数据格式错误：" + rec_data);
                }
                else
                {
                    Logger("Recv -> " + rec_data);
                }

                // 数据处理
                parse_tempture(revData);
                parse_illumination(revData);
            }
        }

        private void parse_tempture(byte[] _Bytes)
        {
            // 判断数据是否正确
            if (_Bytes[0] == command.REC_START && _Bytes[1] == command.WRITE && _Bytes[2] == command.TEMPTURE)
            {
                double a = ((_Bytes[3] << 24) + (_Bytes[4] << 16) + (_Bytes[5] << 8) + _Bytes[6]) / 10000.0;
                textBox1.Text = a.ToString() + "℃";
            }
        }

        private void parse_illumination(byte[] _Bytes)
        {
            // 判断数据是否正确
            if (_Bytes[0] == command.REC_START && _Bytes[1] == command.WRITE && _Bytes[2] == command.ILLUMINA)
            {
                int a = _Bytes[6];
                textBox2.Text = a.ToString();
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            selectAndAddComport(serialPort1);
        }

        private void get_tempture()
        {
            // 查询温度指令
            sendSerialData(command.GetCommand(Mode.TEMPTURE));
        }

        private void get_illumination()
        {
            // 查询温度指令
            sendSerialData(command.GetCommand(Mode.ILLUMINA));
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (device)
                get_tempture();
            else
                get_illumination();

            device = !device;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Logger(button1.Text);

            if (serialPort1.IsOpen)
            {
                if (isStart)
                {
                    isStart = false;
                    timer1.Enabled = false;
                    button5.Enabled = true;
                    button6.Enabled = true;
                    button7.Enabled = true;
                    button2.Enabled = true;
                    button1.Text = "开始监测";
                }
                else
                {
                    isStart = true;
                    timer1.Enabled = true;
                    button5.Enabled = false;
                    button6.Enabled = false;
                    button7.Enabled = false;
                    button2.Enabled = false;
                    button1.Text = "停止监测";
                }
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            if (!serialPort1.IsOpen)
            {
                if (string.IsNullOrWhiteSpace(comboBox1.Text))  // 空判断
                {
                    MessageBox.Show("请先搜索端口", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                serialPort1.PortName = comboBox1.Text;
                serialPort1.BaudRate = 115200;
                serialPort1.Parity = Parity.None;
                try
                {
                    serialPort1.Open();
                    Logger("串口打开成功");
                    button1.Enabled = true;
                    button3.Enabled = false;
                    button4.Text = "关闭串口";
                }
                catch (Exception ex)
                {
                    serialPort1.Close();
                    MessageBox.Show("串口打开失败, " + ex.Message.ToString(), "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
            {
                Logger("串口关闭成功");
                serialPort1.Close();
                button4.Text = "打开串口";
                timer1.Enabled = false;
                button3.Enabled = true;
                button1.Enabled = false;
            }
        }

        private void button5_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)  // 串口打开了
            {
                button5.Enabled = false;
                get_tempture();
                button5.Enabled = true;
            }
            else
            {
                MessageBox.Show("请先打开串口", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void button6_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)  // 串口打开了
            {
                button6.Enabled = false;
                get_illumination();
                button6.Enabled = true;
            }
            else
            {
                MessageBox.Show("请先打开串口", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void button7_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)  // 串口打开了
            {
                if (textBox5.Text == string.Empty)
                {
                    MessageBox.Show("请输入范围", "警告", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else
                {
                    if (textBox5.Text.Contains('-'))
                    {
                        string[] val = textBox5.Text.Split('-');
                        var min = int.Parse(val[0]);
                        var max = int.Parse(val[1]);

                        if (max < min || max < 0 || max > 250 || min < 0 || min > 250)
                        {
                            MessageBox.Show("输入范围错误，请输入0~250的整数", "警告", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        }
                        else
                        {
                            sendSerialData(command.GetSetCommand(Mode.TEMPTURE, max, min));
                        }
                    }
                    else
                    {
                        MessageBox.Show("输入格式错误，最小值-最大值", "警告", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }
            }
            else
            {
                MessageBox.Show("请先打开串口", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)  // 串口打开了
            {
                if (textBox4.Text == string.Empty)
                {
                    MessageBox.Show("请输入范围", "警告", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else
                {
                    if (textBox4.Text.Contains('-'))
                    {
                        string[] val = textBox4.Text.Split('-');
                        var min = int.Parse(val[0]);
                        var max = int.Parse(val[1]);

                        if (max < min || max < 0 || max > 250 || min < 0 || min > 250)
                        {
                            MessageBox.Show("输入范围错误，请输入0~250的整数", "警告", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        }
                        else
                        {
                            sendSerialData(command.GetSetCommand(Mode.ILLUMINA, max, min));
                        }
                    }
                    else
                    {
                        MessageBox.Show("输入格式错误，最小值-最大值", "警告", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }
            }
            else
            {
                MessageBox.Show("请先打开串口", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
