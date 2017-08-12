using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Security.Cryptography;
using System.IO;

namespace WCPortFwd
{
	public partial class EditWin : Form
	{
		private ForwardInfo FI;

		public EditWin(ForwardInfo fi)
		{
			this.FI = fi;

			InitializeComponent();
		}

		private void EditWin_Load(object sender, EventArgs e)
		{
			this.Icon = Gnd.I.MainWin.Icon;
			this.LoadInfo();
		}

		private void EditWin_FormClosed(object sender, FormClosedEventArgs e)
		{
			this.SaveInfo();
		}

		private void LoadInfo()
		{
			this.RecvPortNo.Text = "" + this.FI.RecvPortNo;
			this.ForwardPortNo.Text = "" + this.FI.ForwardPortNo;
			this.ForwardDomain.Text = "" + this.FI.ForwardDomain;
			this.ConnectMax.Text = "" + this.FI.ConnectMax;

			if (this.FI.RawKey == "")
			{
				this.RB暗号化ナシ.Checked = true;
			}
			else if (this.FI.DecMode)
			{
				this.RB復号.Checked = true;
			}
			else
			{
				this.RB暗号化.Checked = true;
			}
			this.RawKey.Text = "";
			this.RawKey.Text = this.FI.RawKey;
		}

		private void SaveInfo()
		{
			this.FI.RecvPortNo = Tools.Range(Tools.Parse(this.RecvPortNo.Text, 8080), 1, 65535);
			this.FI.ForwardPortNo = Tools.Range(Tools.Parse(this.ForwardPortNo.Text, 80), 1, 65535);
			this.FI.ForwardDomain = Tools.DomainFltr(this.ForwardDomain.Text);
			this.FI.ConnectMax = Tools.Range(Tools.Parse(this.ConnectMax.Text, 10), 1, 500);

			if (this.RB暗号化ナシ.Checked)
			{
				this.FI.DecMode = false;
				this.FI.RawKey = "";
			}
			else
			{
				this.FI.DecMode = this.RB復号.Checked;
				this.FI.RawKey = Tools.PassphraseFltr(this.RawKey.Text);

				if (this.FI.RawKey == "")
					this.FI.RawKey = Gnd.I.DEFAULT_RAWKEY;
			}
		}

		private void Btn自動生成_Click(object sender, EventArgs e)
		{
			byte[] byteData = new byte[64];

			using (RNGCryptoServiceProvider r = new RNGCryptoServiceProvider())
			{
				r.GetBytes(byteData);
			}
			StringBuilder sBuff = new StringBuilder();

			foreach (byte chr in byteData)
			{
#if true
				sBuff.Append(chr.ToString("x2"));
#else // OLD
				sBuff.Append((Gnd.I.DIGIT + Gnd.I.alpha)[(int)chr / 16]);
				sBuff.Append((Gnd.I.DIGIT + Gnd.I.alpha)[(int)chr % 16]);
#endif
			}
			this.RawKey.Text = "" + sBuff;
		}

		private void RB暗号化ナシ_CheckedChanged(object sender, EventArgs e)
		{
			bool ctrlOn = this.RB暗号化ナシ.Checked == false;

			this.RawKey.Enabled = ctrlOn;
			this.Btn自動生成.Enabled = ctrlOn;
			this.Btnファイルに保存.Enabled = ctrlOn;
			this.Btnファイルから読み込み.Enabled = ctrlOn;
			this.Btnクリア.Enabled = ctrlOn;
			this.RawKeyInfo.Enabled = ctrlOn;
			this.RawKeyLabel.Enabled = ctrlOn;
		}

		private void BtnReset_Click(object sender, EventArgs e)
		{
			this.LoadInfo();
		}

		private void RawKey_TextChanged(object sender, EventArgs e)
		{
			string newRawKey = Tools.PassphraseFltr(this.RawKey.Text);

			if (this.RawKey.Text != newRawKey)
			{
				this.RawKey.Text = newRawKey;
				this.RawKey.SelectionStart = newRawKey.Length;
			}
			string info = "";

			if (Tools.IsHex128(this.RawKey.Text))
				info = "hex 512-bit";

			this.RawKeyInfo.Text = info;
		}

		private void RawKey_KeyPress(object sender, KeyPressEventArgs e)
		{
			char CTRL_A = (char)1;

			if (e.KeyChar == CTRL_A)
			{
				this.RawKey.SelectAll();
				e.Handled = true;
			}
			this.CtrlKeyPress(e);
		}

		private void CtrlKeyPress(KeyPressEventArgs e)
		{
			if (e.KeyChar == (char)Keys.Enter)
			{
				this.Close();
				e.Handled = true;
			}
		}

		private void RecvPortNo_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void ForwardPortNo_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void ForwardDomain_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void ConnectMax_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void RB暗号化_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void RB復号_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void RB暗号化ナシ_KeyPress(object sender, KeyPressEventArgs e)
		{
			this.CtrlKeyPress(e);
		}

		private void Btnファイルから読み込み_Click(object sender, EventArgs e)
		{
			try
			{
				// ---- open dlg ----

				//OpenFileDialogクラスのインスタンスを作成
				using (OpenFileDialog ofd = new OpenFileDialog())
				{
					//はじめのファイル名を指定する
					//はじめに「ファイル名」で表示される文字列を指定する
					//ofd.FileName = "default.html";
					ofd.FileName = "*.rawkey";
					//はじめに表示されるフォルダを指定する
					//指定しない（空の文字列）の時は、現在のディレクトリが表示される
					//ofd.InitialDirectory = @"C:\";
					ofd.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
					//[ファイルの種類]に表示される選択肢を指定する
					//指定しないとすべてのファイルが表示される
					ofd.Filter =
						//"HTMLファイル(*.html;*.htm)|*.html;*.htm|すべてのファイル(*.*)|*.*";
						"鍵ファイル(*.rawkey)|*.rawkey|すべてのファイル(*.*)|*.*";
					//[ファイルの種類]ではじめに
					//「すべてのファイル」が選択されているようにする
					//ofd.FilterIndex = 2;
					ofd.FilterIndex = 1;
					//タイトルを設定する
					ofd.Title = "開くファイルを選択してください";
					//ダイアログボックスを閉じる前に現在のディレクトリを復元するようにする
					ofd.RestoreDirectory = true;
					//存在しないファイルの名前が指定されたとき警告を表示する
					//デフォルトでTrueなので指定する必要はない
					ofd.CheckFileExists = true;
					//存在しないパスが指定されたとき警告を表示する
					//デフォルトでTrueなので指定する必要はない
					ofd.CheckPathExists = true;

					//ダイアログを表示する
					if (ofd.ShowDialog() == DialogResult.OK) // using ofd
					{
						//OKボタンがクリックされたとき
						//選択されたファイル名を表示する
						//Console.WriteLine(ofd.FileName);

						XNode root = XNode.Load(ofd.FileName);
						string rawKey = root.GetChild("CPortFwd").GetChild("RawKey").Text;
						string rawKeyCheckSum = Tools.GetCheckSum(rawKey);
						string read_rawKeyCheckSum = root.GetChild("CPortFwd").GetChild("RawKey-SUM").Text;

						if (read_rawKeyCheckSum != rawKeyCheckSum)
						{
							throw new Exception("【チェックサムが一致しません】");
						}
						rawKey = Tools.PassphraseFltr(rawKey);
						this.RawKey.Text = rawKey;

						MessageBox.Show(
							"[鍵またはパスフレーズ]を読み込みました。",
							"確認",
							MessageBoxButtons.OK,
							MessageBoxIcon.Information
							);
					}
				}

				// ----
			}
			catch (Exception ex)
			{
				MessageBox.Show(
					"鍵ファイルの読み込みに失敗しました。\n" + ex,
					"エラー",
					MessageBoxButtons.OK,
					MessageBoxIcon.Error
					);
			}
		}

		private void Btnファイルに保存_Click(object sender, EventArgs e)
		{
			try
			{
				string rawKey = this.RawKey.Text;

				if (rawKey == "")
				{
					throw new Exception("[鍵またはパスフレーズ]を入力して下さい。");
				}
				string rawKeyCheckSum = Tools.GetCheckSum(rawKey);

				// ---- save dlg ----

				//SaveFileDialogクラスのインスタンスを作成
				using (SaveFileDialog sfd = new SaveFileDialog())
				{
					//はじめのファイル名を指定する
					sfd.FileName = "CPortFwd_KeyOrPass_" + Tools.GetCompactStamp(DateTime.Now) + ".rawkey";
					//はじめに表示されるフォルダを指定する
					//sfd.InitialDirectory = @"C:\";
					sfd.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
					//[ファイルの種類]に表示される選択肢を指定する
					sfd.Filter =
						//"HTMLファイル(*.html;*.htm)|*.html;*.htm|すべてのファイル(*.*)|*.*";
						"鍵ファイル(*.rawkey)|*.rawkey|すべてのファイル(*.*)|*.*";
					//[ファイルの種類]ではじめに
					//「すべてのファイル」が選択されているようにする
					//sfd.FilterIndex = 2;
					sfd.FilterIndex = 1;
					//タイトルを設定する
					sfd.Title = "保存先のファイルを選択してください";
					//ダイアログボックスを閉じる前に現在のディレクトリを復元するようにする
					sfd.RestoreDirectory = true;
					//既に存在するファイル名を指定したとき警告する
					//デフォルトでTrueなので指定する必要はない
					sfd.OverwritePrompt = true;
					//存在しないパスが指定されたとき警告を表示する
					//デフォルトでTrueなので指定する必要はない
					sfd.CheckPathExists = true;

					//ダイアログを表示する
					if (sfd.ShowDialog() == DialogResult.OK) // using sfd
					{
						//OKボタンがクリックされたとき
						//選択されたファイル名を表示する
						//Console.WriteLine(sfd.FileName);

						List<string> lines = new List<string>();

						lines.Add("<?xml version=\"1.0\" encoding=\"Shift_JIS\"?>");
						lines.Add("<Root>");
						lines.Add("<CPortFwd>");
						lines.Add("<RawKey>");
						lines.Add(rawKey);
						lines.Add("</RawKey>");
						lines.Add("<RawKey-SUM>");
						lines.Add(rawKeyCheckSum);
						lines.Add("</RawKey-SUM>");
						lines.Add("</CPortFwd>");
						lines.Add("</Root>");

						File.WriteAllLines(sfd.FileName, lines, Encoding.GetEncoding(932));

						MessageBox.Show(
							"[鍵またはパスフレーズ]を保存しました。",
							"確認",
							MessageBoxButtons.OK,
							MessageBoxIcon.Information
							);
					}
				}

				// ----
			}
			catch (Exception ex)
			{
				MessageBox.Show(
					"鍵ファイルの保存に失敗しました。\n" + ex,
					"エラー",
					MessageBoxButtons.OK,
					MessageBoxIcon.Error
					);
			}
		}

		private void Btnクリア_Click(object sender, EventArgs e)
		{
			this.RawKey.Text = "";
		}
	}
}
