using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;

// Conversor da bancada: os .bgra_dumpados pelo OrbPreview viram PNG (Format32bppArgb usa a
// mesma ordem de bytes BGRA), para que o padrao do orbe possa ser conferido sem o CS2.
internal static class ToPng
{
    private static int Main(string[] args)
    {
        int ok = 0;
        foreach (string path in args)
        {
            byte[] bytes = File.ReadAllBytes(path);
            int side = (int)Math.Round(Math.Sqrt(bytes.Length / 4.0));
            if (side <= 0 || side * side * 4 != bytes.Length)
            {
                Console.WriteLine("tamanho inesperado: " + path + " (" + bytes.Length + " bytes)");
                continue;
            }
            using (Bitmap bmp = new Bitmap(side, side, PixelFormat.Format32bppArgb))
            {
                BitmapData data = bmp.LockBits(new Rectangle(0, 0, side, side), ImageLockMode.WriteOnly,
                    PixelFormat.Format32bppArgb);
                try
                {
                    if (data.Stride == side * 4)
                    {
                        Marshal.Copy(bytes, 0, data.Scan0, bytes.Length);
                    }
                    else
                    {
                        for (int y = 0; y < side; y++)
                        {
                            IntPtr row = IntPtr.Add(data.Scan0, y * data.Stride);
                            Marshal.Copy(bytes, y * side * 4, row, side * 4);
                        }
                    }
                }
                finally { bmp.UnlockBits(data); }
                string target = Path.ChangeExtension(path, ".png");
                bmp.Save(target, ImageFormat.Png);
                Console.WriteLine("png=" + target + " " + side + "x" + side);
                ok++;
            }
        }
        return ok == args.Length ? 0 : 1;
    }
}
