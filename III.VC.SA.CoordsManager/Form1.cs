using System;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Linq;
using System.Globalization;

namespace III.VC.SA.CoordsManager
{
    public partial class Form1 : Form
    {
        private IntPtr radar_player_target;
        private IntPtr gta3ico;
        private IntPtr gtavcico;
        private IntPtr gtasaico;
        System.ComponentModel.BackgroundWorker bw = new System.ComponentModel.BackgroundWorker();

        public Form1()
        {
            InitializeComponent();
            radar_player_target = Properties.Resources.radar_player_target.GetHicon();
            gta3ico = Properties.Resources.gta3.GetHicon();
            gtavcico = Properties.Resources.gtavc.GetHicon();
            gtasaico = Properties.Resources.gtasa.GetHicon();

            this.Icon = System.Drawing.Icon.FromHandle(radar_player_target);

            bw.DoWork += ProcessCheck;
            bw.RunWorkerAsync();
        }

        void ProcessCheck(object sender, System.ComponentModel.DoWorkEventArgs e)
        {
            while (true)
            {
                if (minjector.GetProcess() == null)
                {
                    var captions = new[] { "gta", "GTA", "Grand Theft Auto", "grand theft auto" };
                    Process[] processlist = Process.GetProcesses();
                    foreach (Process pr in processlist)
                    {
                        if (captions.Any(pr.MainWindowTitle.StartsWith))
                        {
                            minjector.SetProcess(pr);
                            if (GameVersion.DetectGameVersion())
                            {
                                Game.pPlayer = UIntPtr.Zero;
                                Game.pCar = UIntPtr.Zero;
                                Game.PlayerPed = UIntPtr.Zero;
                                MethodInvoker mi = null;
                                if (GameVersion.IsIII())
                                {
                                    mi = delegate () { this.Icon = System.Drawing.Icon.FromHandle(gta3ico); };
                                    Game.pPlayer = (UIntPtr)0x9412F0;
                                    Game.pCar = (UIntPtr)0x8F5FE8;
                                }
                                else
                                {
                                    if (GameVersion.IsVC())
                                    {
                                        mi = delegate () { this.Icon = System.Drawing.Icon.FromHandle(gtavcico); };
                                        Game.pPlayer = (UIntPtr)0x94AD28;
                                        Game.pCar = (UIntPtr)0x9B6A80;
                                    }
                                    else
                                    {
                                        if (GameVersion.IsSA())
                                        {
                                            mi = delegate () { this.Icon = System.Drawing.Icon.FromHandle(gtasaico); };
                                            Game.pPlayer = (UIntPtr)0xB7CD98;
                                            Game.pCar = (UIntPtr)0xBA18FC;
                                        }
                                    }
                                }
                                this.Invoke(mi);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    if (Game.PlayerPed == UIntPtr.Zero)
                    {
                        Game.PlayerPed = (UIntPtr)minjector.ReadMemory((uint)Game.pPlayer, true);
                        Game.PlayerCar = (UIntPtr)minjector.ReadMemory((uint)Game.pCar, true);
                        if (GameVersion.IsSA())
                        {
                            UIntPtr pCoords;
                            if (Game.PlayerCar != UIntPtr.Zero)
                                pCoords = (UIntPtr)minjector.ReadMemory((uint)Game.PlayerCar + 0x14, true);
                            else
                                pCoords = (UIntPtr)minjector.ReadMemory((uint)Game.PlayerPed + 0x14, true);

                            Game.pPlayerPosX = pCoords + 0x30;
                            Game.pPlayerPosY = pCoords + 0x34;
                            Game.pPlayerPosZ = pCoords + 0x38;
                        }
                        else
                        {
                            if (Game.PlayerCar == UIntPtr.Zero)
                            {
                                Game.pPlayerPosX = Game.PlayerPed + 0x34;
                                Game.pPlayerPosY = Game.PlayerPed + 0x38;
                                Game.pPlayerPosZ = Game.PlayerPed + 0x3C;
                            }
                            else
                            {
                                Game.pPlayerPosX = Game.PlayerCar + 0x34;
                                Game.pPlayerPosY = Game.PlayerCar + 0x38;
                                Game.pPlayerPosZ = Game.PlayerCar + 0x3C;
                            }
                        }
                    }

                    if (checkBox1.Checked)
                    {
                        ReadCoords();
                    }
                }
            }
        }

        private void ReadCoords()
        {
            float fPosX = minjector.ReadMemory<float>((uint)Game.pPlayerPosX, true);
            float fPosY = minjector.ReadMemory<float>((uint)Game.pPlayerPosY, true);
            float fPosZ = minjector.ReadMemory<float>((uint)Game.pPlayerPosZ, true);

            if (fPosX != 0.0f && fPosY != 0.0f && fPosZ != 0.0f)
            {
                string PosX = fPosX.ToString("0.0000", CultureInfo.InvariantCulture.NumberFormat);
                string PosY = fPosY.ToString("0.0000", CultureInfo.InvariantCulture.NumberFormat);
                string PosZ = fPosZ.ToString("0.0000", CultureInfo.InvariantCulture.NumberFormat);

                MethodInvoker mi = null;
                mi = delegate ()
                {
                    textBox1.Text = PosX + " " + PosY + " " + PosZ;
                    //textBox2.Text = PosX;
                    //textBox3.Text = PosY;
                    //textBox4.Text = PosZ;
                };
                this.Invoke(mi);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            ReadCoords();
        }

        private void checkBox1_MouseHover(object sender, EventArgs e)
        {
            toolTip1.SetToolTip(checkBox1, "Autoupdate");
        }

        private void textBoxes_Enter(object sender, EventArgs e)
        {
            checkBox1.Checked = false;
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (textBox2.Text != "" && textBox3.Text != "" && textBox4.Text != "")
            {
                float fPosX = float.Parse(textBox2.Text, CultureInfo.InvariantCulture.NumberFormat);
                float fPosY = float.Parse(textBox3.Text, CultureInfo.InvariantCulture.NumberFormat);
                float fPosZ = float.Parse(textBox4.Text, CultureInfo.InvariantCulture.NumberFormat);

                minjector.WriteMemory<float>((uint)Game.pPlayerPosX, fPosX, true);
                minjector.WriteMemory<float>((uint)Game.pPlayerPosY, fPosY, true);
                minjector.WriteMemory<float>((uint)Game.pPlayerPosZ, fPosZ, true);
            }
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            string[] str = textBox1.Text.Split(' ');

            if (str.Length == 3)
            {
                textBox2.Text = str[0];
                textBox3.Text = str[1];
                textBox4.Text = str[2];
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (textBox2.Text != "" && textBox3.Text != "" && textBox4.Text != "")
                Clipboard.SetText(textBox2.Text + " " + textBox3.Text + " " + textBox4.Text);
        }
    }
}
