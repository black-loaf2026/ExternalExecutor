using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace ExternalExecutorUI
{
    public partial class Form1 : Form
    {
        [DllImport("ExternalExecutor.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Nuke(string input);

        public Form1()
        {
            InitializeComponent();
        }

        bool Injected = false;
        private void button2_Click(object sender, EventArgs e)
        {
            Injected = true;
            Nuke("");
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (Injected)
            {
                Nuke(richTextBox1.Text);
            }
            else
            {
                MessageBox.Show("Inject first!");
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            richTextBox1.Text = "";
        }
    }
}
