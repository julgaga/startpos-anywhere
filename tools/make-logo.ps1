param([string]$Out)
Add-Type -AssemblyName System.Drawing

$S = 336
$bmp = New-Object System.Drawing.Bitmap $S, $S
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::Transparent)

function RoundRect([float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $p.AddArc($x, $y, $d, $d, 180, 90)
    $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}
function C([int]$r, [int]$gr, [int]$b, [int]$a = 255) { [System.Drawing.Color]::FromArgb($a, $r, $gr, $b) }

# Fond : carré arrondi en dégradé violet -> bleu
$bg = RoundRect 8 8 320 320 56
$grad = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.PointF 0, 0), (New-Object System.Drawing.PointF $S, $S), (C 58 22 110), (C 22 92 170)
$g.FillPath($grad, $bg)
$g.DrawPath((New-Object System.Drawing.Pen (C 255 255 255 60), 6), $bg)

# Barre de progression
$trackX = 48; $trackY = 236; $trackW = 240; $trackH = 30
$track = RoundRect $trackX $trackY $trackW $trackH 15
$g.FillPath((New-Object System.Drawing.SolidBrush (C 10 10 30 170)), $track)
$fillW = [int]($trackW * 0.5)
$fill = RoundRect $trackX $trackY $fillW $trackH 15
$g.FillPath((New-Object System.Drawing.SolidBrush (C 92 230 90)), $fill)
$g.DrawPath((New-Object System.Drawing.Pen (C 255 255 255 200), 4), $track)

# Curseur du slider
$tx = $trackX + $fillW; $ty = $trackY + $trackH / 2
$g.FillEllipse((New-Object System.Drawing.SolidBrush (C 255 255 255)), $tx - 22, $ty - 22, 44, 44)
$g.DrawEllipse((New-Object System.Drawing.Pen (C 20 20 40), 5), $tx - 22, $ty - 22, 44, 44)

# Trait pointillé du curseur vers la start pos
$dash = New-Object System.Drawing.Pen (C 255 255 255 200), 6
$dash.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash
$g.DrawLine($dash, $tx, 196, $tx, $ty - 26)

# Start pos : triangle « play » vert avec contour
$cx = $tx - 8; $cy = 128   # le centre visuel d'un triangle « play » est à gauche de sa pointe
$tri = New-Object System.Drawing.Drawing2D.GraphicsPath
$pts = [System.Drawing.PointF[]]@(
    (New-Object System.Drawing.PointF ($cx - 46), ($cy - 58)),
    (New-Object System.Drawing.PointF ($cx + 62), $cy),
    (New-Object System.Drawing.PointF ($cx - 46), ($cy + 58))
)
$tri.AddPolygon($pts)
$outline = New-Object System.Drawing.Pen (C 20 20 40), 14
$outline.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
$g.DrawPath($outline, $tri)
$triGrad = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.PointF 0, ($cy - 58)), (New-Object System.Drawing.PointF 0, ($cy + 58)), (C 150 255 120), (C 40 190 70)
$g.FillPath($triGrad, $tri)
$shine = New-Object System.Drawing.Pen (C 255 255 255 150), 5
$shine.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
$g.DrawLine($shine, $cx - 34, $cy - 36, $cx - 34, $cy + 20)

$g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
"saved $Out"
