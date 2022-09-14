using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UART_SENSOR
{
    enum Mode
    {
        TEMPTURE,
        ILLUMINA
    }
    internal class command
    {

        public const byte TEMPTURE = 0x00;
        public const byte ILLUMINA = 0x01;
        public const byte SED_START = 0xFF;
        public const byte REC_START = 0xEE;
        public const byte READ = 0x00;
        public const byte WRITE = 0x01;
        public const byte NULL = 0x00;

        public static string GetCommand(Mode mode)
        {
            switch (mode)
            {
                case Mode.TEMPTURE:
                    return ("0xFF 0x00 0x00 0x00 0x00 0x00 0x00 0x00");
                case Mode.ILLUMINA:
                    return ("0xFF 0x00 0x01 0x00 0x00 0x00 0x00 0x00");
                default:
                    return ("0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00");
            }
        }
    }
}
