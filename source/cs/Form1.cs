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
                                Game.nInCarOffset = 0;
                                MethodInvoker mi = null;
                                if (GameVersion.IsIII())
                                {
                                    mi = delegate () { this.Icon = System.Drawing.Icon.FromHandle(gta3ico); };
                                    Game.pPlayer = (UIntPtr)0x9412F0;
                                    Game.pCar = (UIntPtr)0x8F5FE8;
                                    Game.nInCarOffset = 0x314;
                                }
                                else
                                {
                                    if (GameVersion.IsVC())
                                    {
                                        mi = delegate () { this.Icon = System.Drawing.Icon.FromHandle(gtavcico); };
                                        Game.pPlayer = (UIntPtr)0x94AD28;
                                        Game.pCar = (UIntPtr)0x9B6A80;
                                        Game.nInCarOffset = 0x3AC;
                                    }
                                    else
                                    {
                                        if (GameVersion.IsSA())
                                        {
                                            mi = delegate () { this.Icon = System.Drawing.Icon.FromHandle(gtasaico); };
                                            Game.pPlayer = (UIntPtr)0xB7CD98;
                                            Game.pCar = (UIntPtr)0xBA18FC;
                                            Game.nInCarOffset = 0x46C;
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
                    if (checkBox1.Checked)
                    {
                        ReadCoords();
                    }
                }
            }
        }

        private void UpdateAddresses()
        {
            Game.PlayerPed = minjector.ReadMemory<UIntPtr>(Game.pPlayer, true);
            Game.PlayerCar = minjector.ReadMemory<UIntPtr>(Game.pCar, true);
            bool bIsPedInCar = minjector.ReadMemory<Byte>((UInt32)Game.PlayerPed + Game.nInCarOffset, true) == 1;
            if (GameVersion.IsSA())
            {
                UIntPtr pCoords = UIntPtr.Zero;
                if (!bIsPedInCar || Game.PlayerCar == UIntPtr.Zero)
                    pCoords = minjector.ReadMemory<UIntPtr>(Game.PlayerPed + 0x14, true);
                else
                    pCoords = minjector.ReadMemory<UIntPtr>(Game.PlayerCar + 0x14, true);

                Game.pPlayerPosX = pCoords + 0x30;
                Game.pPlayerPosY = pCoords + 0x34;
                Game.pPlayerPosZ = pCoords + 0x38;
            }
            else
            {
                if (!bIsPedInCar)
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

        private void ReadCoords()
        {
            UpdateAddresses();

            float fPosX = minjector.ReadMemory<float>(Game.pPlayerPosX, true);
            float fPosY = minjector.ReadMemory<float>(Game.pPlayerPosY, true);
            float fPosZ = minjector.ReadMemory<float>(Game.pPlayerPosZ, true);

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

                UpdateAddresses();

                minjector.WriteMemory<float>(Game.pPlayerPosX, fPosX, true);
                minjector.WriteMemory<float>(Game.pPlayerPosY, fPosY, true);
                minjector.WriteMemory<float>(Game.pPlayerPosZ, fPosZ, true);
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
