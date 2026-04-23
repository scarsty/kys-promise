unit kys_gfx;

{$mode Delphi}

interface

uses
  SDL3, Classes, SysUtils, Math;

const
  VALUE_LIMIT = 0.001;
  GUARD_ROWS = 2;

function rotozoomSurfaceXY(src: PSDL_Surface; angle, zoomx, zoomy: real; smooth: integer): PSDL_Surface;

implementation

uses
  kys_engine;

function SDL_MUSTLOCK(src: PSDL_Surface): boolean;
begin
  Result := False;
end;

procedure _rotozoomSurfaceSizeTrig(Width, Height: integer; angle, zoomx, zoomy: double; var dstwidth, dstheight: integer; var canglezoom, sanglezoom: double);
var
  x, y, cx, cy, sx, sy: double;
  radangle: double;
  dstwidthhalf, dstheighthalf: integer;
begin
  // Determine destination width and height by rotating a centered source box
  radangle := angle * (Pi / 180.0);
  sanglezoom := Sin(radangle);
  canglezoom := Cos(radangle);
  sanglezoom := sanglezoom * zoomx;
  canglezoom := canglezoom * zoomy;

  x := Width / 2.0;
  y := Height / 2.0;

  cx := canglezoom * x;
  cy := canglezoom * y;
  sx := sanglezoom * x;
  sy := sanglezoom * y;

  dstwidthhalf := Max(Ceil(Max(Max(Max(Abs(cx + sy), Abs(cx - sy)), Abs(-cx + sy)), Abs(-cx - sy))), 1);

  dstheighthalf := Max(Ceil(Max(Max(Max(Abs(sx + cy), Abs(sx - cy)), Abs(-sx + cy)), Abs(-sx - cy))), 1);

  dstwidth := 2 * dstwidthhalf;
  dstheight := 2 * dstheighthalf;
end;


procedure _transformSurfaceRGBA(src, dst: PSDL_Surface; cx, cy, isin, icos, flipx, flipy, smooth: integer);
var
  x, y, t1, t2, dx, dy, xd, yd, sdx, sdy, ax, ay, ex, ey, sw, sh: integer;
  c00, c01, c10, c11, cswap: TSDL_Color;
  pc, sp: PSDL_Color;
  gap: integer;
begin
  // Variable setup
  xd := (src^.w - dst^.w) shl 15;
  yd := (src^.h - dst^.h) shl 15;
  ax := (cx shl 16) - (icos * cx);
  ay := (cy shl 16) - (isin * cx);
  sw := src^.w - 1;
  sh := src^.h - 1;
  pc := PSDL_Color(dst^.pixels);
  gap := dst^.pitch - dst^.w * 4;

  // Interpolating branch
  if smooth <> 0 then
  begin
    for y := 0 to dst^.h - 1 do
    begin
      dy := cy - y;
      sdx := ax + (isin * dy) + xd;
      sdy := ay - (icos * dy) + yd;

      for x := 0 to dst^.w - 1 do
      begin
        dx := sdx shr 16;
        dy := sdy shr 16;
        if flipx <> 0 then dx := sw - dx;
        if flipy <> 0 then dy := sh - dy;

        if (dx >= 0) and (dy >= 0) and (dx < src^.w - 1) and (dy < src^.h - 1) then
        begin
          sp := PSDL_Color(src^.pixels);
          Inc(sp, (src^.pitch div 4) * dy + dx);
          c00 := sp^;
          Inc(sp);
          c01 := sp^;
          Inc(sp, src^.pitch div 4);
          c11 := sp^;
          Dec(sp);
          c10 := sp^;

          if flipx <> 0 then
          begin
            cswap := c00;
            c00 := c01;
            c01 := cswap;
            cswap := c10;
            c10 := c11;
            c11 := cswap;
          end;
          if flipy <> 0 then
          begin
            cswap := c00;
            c00 := c10;
            c10 := cswap;
            cswap := c01;
            c01 := c11;
            c11 := cswap;
          end;

          ex := sdx and $FFFF;
          ey := sdy and $FFFF;

          // Interpolation
          t1 := (((c01.r - c00.r) * ex) shr 16 + c00.r) and $FF;
          t2 := (((c11.r - c10.r) * ex) shr 16 + c10.r) and $FF;
          pc^.r := ((t2 - t1) * ey shr 16) + t1;

          t1 := (((c01.g - c00.g) * ex) shr 16 + c00.g) and $FF;
          t2 := (((c11.g - c10.g) * ex) shr 16 + c10.g) and $FF;
          pc^.g := ((t2 - t1) * ey shr 16) + t1;

          t1 := (((c01.b - c00.b) * ex) shr 16 + c00.b) and $FF;
          t2 := (((c11.b - c10.b) * ex) shr 16 + c10.b) and $FF;
          pc^.b := ((t2 - t1) * ey shr 16) + t1;

          t1 := (((c01.a - c00.a) * ex) shr 16 + c00.a) and $FF;
          t2 := (((c11.a - c10.a) * ex) shr 16 + c10.a) and $FF;
          pc^.a := ((t2 - t1) * ey shr 16) + t1;
        end;

        sdx += icos;
        sdy += isin;
        Inc(pc);
      end;
      pc := PSDL_Color(pbyte(pc) + gap);
    end;
  end
  else
  begin
    // Non-interpolating branch
    for y := 0 to dst^.h - 1 do
    begin
      dy := cy - y;
      sdx := ax + (isin * dy) + xd;
      sdy := ay - (icos * dy) + yd;

      for x := 0 to dst^.w - 1 do
      begin
        dx := sdx shr 16;
        dy := sdy shr 16;
        if flipx <> 0 then dx := sw - dx;
        if flipy <> 0 then dy := sh - dy;

        if (dx >= 0) and (dy >= 0) and (dx < src^.w) and (dy < src^.h) then
        begin
          sp := PSDL_Color(pbyte(src^.pixels) + src^.pitch * dy);
          Inc(sp, dx);
          pc^ := sp^;
        end;

        sdx += icos;
        sdy += isin;
        Inc(pc);
      end;
      pc := PSDL_Color(pbyte(pc) + gap);
    end;
  end;
end;

procedure zoomSurfaceSize(Width, Height: integer; zoomx, zoomy: double; var dstwidth, dstheight: integer);
var
  flipx, flipy: integer;
begin
  // Make zoom factors positive
  flipx := Ord(zoomx < 0.0);
  if flipx <> 0 then zoomx := -zoomx;

  flipy := Ord(zoomy < 0.0);
  if flipy <> 0 then zoomy := -zoomy;

  // Sanity check zoom factors
  if zoomx < VALUE_LIMIT then zoomx := VALUE_LIMIT;
  if zoomy < VALUE_LIMIT then zoomy := VALUE_LIMIT;

  // Calculate target size
  dstwidth := Round((Width * zoomx) + 0.5);
  dstheight := Round((Height * zoomy) + 0.5);

  if dstwidth < 1 then dstwidth := 1;
  if dstheight < 1 then dstheight := 1;
end;

function _zoomSurfaceRGBA(src, dst: PSDL_Surface; flipx, flipy, smooth: integer): integer;
var
  x, y, sx, sy, ssx, ssy, csx, csy, ex, ey, cx, cy, sstep, sstepx, sstepy: integer;
  sax, say, csax, csay, salast: PInteger;
  c00, c01, c10, c11: TSDL_Color;
  sp, csp, dp: PSDL_Color;
  spixelgap, spixelw, spixelh, dgap, t1, t2: integer;
begin
  sax := AllocMem((dst^.w + 1) * SizeOf(integer));
  say := AllocMem((dst^.h + 1) * SizeOf(integer));
  if (sax = nil) or (say = nil) then
  begin
    if sax <> nil then FreeMem(sax);
    if say <> nil then FreeMem(say);
    Exit(-1);
  end;

  spixelw := src^.w - 1;
  spixelh := src^.h - 1;

  if smooth <> 0 then
  begin
    sx := Round(65536.0 * spixelw / (dst^.w - 1));
    sy := Round(65536.0 * spixelh / (dst^.h - 1));
  end
  else
  begin
    sx := Round(65536.0 * src^.w / dst^.w);
    sy := Round(65536.0 * src^.h / dst^.h);
  end;

  ssx := (src^.w shl 16) - 1;
  ssy := (src^.h shl 16) - 1;

  csx := 0;
  csax := sax;
  for x := 0 to dst^.w do
  begin
    csax^ := Min(csx, ssx);
    Inc(csax);
    csx += sx;
  end;

  csy := 0;
  csay := say;
  for y := 0 to dst^.h do
  begin
    csay^ := Min(csy, ssy);
    Inc(csay);
    csy += sy;
  end;

  sp := PSDL_Color(src^.pixels);
  dp := PSDL_Color(dst^.pixels);
  dgap := dst^.pitch - dst^.w * 4;
  spixelgap := src^.pitch div 4;

  if flipx <> 0 then Inc(sp, spixelw);  //sp := sp + spixelw;
  if flipy <> 0 then Inc(sp, spixelgap * spixelh); //sp := sp + (spixelgap * spixelh);

  if smooth <> 0 then
  begin
    // Interpolating Zoom
    csay := say;
    for y := 0 to dst^.h - 1 do
    begin
      csp := sp;
      csax := sax;
      for x := 0 to dst^.w - 1 do
      begin
        // Setup color source pointers
        ex := csax^ and $FFFF;
        ey := csay^ and $FFFF;
        cx := csax^ shr 16;
        cy := csay^ shr 16;
        sstepx := Ord(cx < spixelw);
        sstepy := Ord(cy < spixelh);

        c00 := sp^;
        c01 := sp^;
        c10 := sp^;

        if sstepy <> 0 then
        begin
          if flipy <> 0 then
            c10 := PSDL_Color(pbyte(sp) - spixelgap * SizeOf(TSDL_Color))^
          else
            c10 := PSDL_Color(pbyte(sp) + spixelgap * SizeOf(TSDL_Color))^;
        end;
        c11 := c10;

        if sstepx <> 0 then
        begin
          if flipx <> 0 then
          begin
            c01 := PSDL_Color(pbyte(@c01) - SizeOf(TSDL_Color))^;
            c11 := PSDL_Color(pbyte(@c11) - SizeOf(TSDL_Color))^;
          end
          else
          begin
            c01 := PSDL_Color(pbyte(@c01) + SizeOf(TSDL_Color))^;
            c11 := PSDL_Color(pbyte(@c11) + SizeOf(TSDL_Color))^;
          end;
        end;

        // Interpolate RGBA channels
        t1 := (((c01.r - c00.r) * ex) shr 16 + c00.r) and $FF;
        t2 := (((c11.r - c10.r) * ex) shr 16 + c10.r) and $FF;
        dp^.r := ((t2 - t1) * ey shr 16) + t1;

        t1 := (((c01.g - c00.g) * ex) shr 16 + c00.g) and $FF;
        t2 := (((c11.g - c10.g) * ex) shr 16 + c10.g) and $FF;
        dp^.g := ((t2 - t1) * ey shr 16) + t1;

        t1 := (((c01.b - c00.b) * ex) shr 16 + c00.b) and $FF;
        t2 := (((c11.b - c10.b) * ex) shr 16 + c10.b) and $FF;
        dp^.b := ((t2 - t1) * ey shr 16) + t1;

        t1 := (((c01.a - c00.a) * ex) shr 16 + c00.a) and $FF;
        t2 := (((c11.a - c10.a) * ex) shr 16 + c10.a) and $FF;
        dp^.a := ((t2 - t1) * ey shr 16) + t1;

        // Advance source pointer x
        salast := csax;
        Inc(csax);
        sstep := (csax^ shr 16) - (salast^ shr 16);
        if flipx <> 0 then
          Inc(sp, -sstep)  //sp := sp - sstep
        else
          Inc(sp, sstep);  //sp := sp + sstep;

        // Advance destination pointer x
        Inc(dp);
      end;

      // Advance source pointer y
      salast := csay;
      Inc(csay);
      sstep := (csay^ shr 16) - (salast^ shr 16);
      sstep := sstep * spixelgap;
      if flipy <> 0 then
      begin
        sp := csp;
        Inc(sp, -sstep);
        //sp := csp - sstep
      end
      else
      begin
        sp := csp;
        Inc(sp, sstep);
        //sp := csp + sstep;
      end;

      // Advance destination pointer y
      dp := PSDL_Color(pbyte(dp) + dgap);
    end;
  end
  else
  begin
    // Non-Interpolating Zoom
    csay := say;
    for y := 0 to dst^.h - 1 do
    begin
      csp := sp;
      csax := sax;
      for x := 0 to dst^.w - 1 do
      begin
        dp^ := sp^;
        salast := csax;
        Inc(csax);
        sstep := (csax^ shr 16) - (salast^ shr 16);
        if flipx <> 0 then sstep := -sstep;
        Inc(sp, sstep);  //sp := sp + sstep;
        Inc(dp);
      end;

      salast := csay;
      Inc(csay);
      sstep := (csay^ shr 16) - (salast^ shr 16);
      sstep := sstep * spixelgap;
      if flipy <> 0 then sstep := -sstep;
      sp := csp;
      Inc(sp, sstep);
      //sp := csp + sstep;

      dp := PSDL_Color(pbyte(dp) + dgap);
    end;
  end;

  FreeMem(sax);
  FreeMem(say);
  Result := 0;
end;

function rotozoomSurfaceXY(src: PSDL_Surface; angle, zoomx, zoomy: real; smooth: integer): PSDL_Surface;
var
  rz_src: PSDL_Surface;
  rz_dst: PSDL_Surface;
  zoominv: double;
  sanglezoom, canglezoom, sanglezoominv, canglezoominv: double;
  dstwidthhalf, dstwidth, dstheighthalf, dstheight: integer;
  is32bit: integer;
  i, src_converted: integer;
  flipx, flipy: integer;
  details: PSDL_PixelFormatDetails;
  pal_dst: PSDL_Palette;
  pal_src: PSDL_Palette;
begin
  //Result := src;
  if src = nil then
    Exit(nil);

  details := SDL_GetPixelFormatDetails(src^.format);
  is32bit := Ord(details^.bits_per_pixel = 32);

  if (is32bit <> 0) or (details^.bits_per_pixel = 8) then
  begin
    rz_src := src;
    src_converted := 0;
  end
  else
  begin
    rz_src := SDL_CreateSurface(src^.w, src^.h, SDL_PIXELFORMAT_RGBA32);
    SDL_BlitSurface(src, nil, rz_src, nil);
    src_converted := 1;
    is32bit := 1;
  end;

  flipx := Ord(zoomx < 0.0);
  if flipx <> 0 then zoomx := -zoomx;

  flipy := Ord(zoomy < 0.0);
  if flipy <> 0 then zoomy := -zoomy;

  if zoomx < VALUE_LIMIT then zoomx := VALUE_LIMIT;
  if zoomy < VALUE_LIMIT then zoomy := VALUE_LIMIT;

  zoominv := 65536.0 / (zoomx * zoomx);

  if Abs(angle) > VALUE_LIMIT then
  begin
    _rotozoomSurfaceSizeTrig(rz_src^.w, rz_src^.h, angle, zoomx, zoomy, dstwidth, dstheight, canglezoom, sanglezoom);
    sanglezoominv := sanglezoom;
    canglezoominv := canglezoom;
    sanglezoominv := sanglezoominv * zoominv;
    canglezoominv := canglezoominv * zoominv;

    dstwidthhalf := dstwidth div 2;
    dstheighthalf := dstheight div 2;

    rz_dst := nil;
    if is32bit <> 0 then
    begin
      rz_dst := SDL_CreateSurface(dstwidth, dstheight + GUARD_ROWS, rz_src^.format);
    end
    else
    begin
      rz_dst := SDL_CreateSurface(dstwidth, dstheight + GUARD_ROWS, SDL_PIXELFORMAT_INDEX8);
      pal_dst := SDL_CreateSurfacePalette(rz_dst);
    end;

    if rz_dst = nil then
      Exit(nil);

    rz_dst^.h := dstheight;

    if SDL_MUSTLOCK(rz_src) then
      SDL_LockSurface(rz_src);

    if is32bit <> 0 then
    begin
      _transformSurfaceRGBA(rz_src, rz_dst, dstwidthhalf, dstheighthalf,
        Round(sanglezoominv), Round(canglezoominv),
        flipx, flipy, smooth);
    end
    else
    begin
      {// Copy palette and colorkey info
      pal_src := SDL_GetSurfacePalette(rz_src);
      for i := 0 to pal_src^.ncolors - 1 do
        pal_dst^.colors[i] := pal_src^.colors[i];
      pal_dst^.ncolors := pal_src^.ncolors;

      // Call the 8bit transformation routine to do the rotation
      transformSurfaceY(rz_src, rz_dst, dstwidthhalf, dstheighthalf,
        Round(sanglezoominv), Round(canglezoominv),
        flipx, flipy);}
    end;

    // Unlock source surface
    if SDL_MUSTLOCK(rz_src) then
      SDL_UnlockSurface(rz_src);
  end
  else
  begin
    zoomSurfaceSize(rz_src^.w, rz_src^.h, zoomx, zoomy, dstwidth, dstheight);

    rz_dst := nil;
    if is32bit <> 0 then
    begin
      rz_dst := SDL_CreateSurface(dstwidth, dstheight + GUARD_ROWS, rz_src^.format);
    end
    else
    begin
      rz_dst := SDL_CreateSurface(dstwidth, dstheight + GUARD_ROWS, SDL_PIXELFORMAT_INDEX8);
      pal_dst := SDL_CreateSurfacePalette(rz_dst);
    end;

    if rz_dst = nil then
      Exit(nil);

    rz_dst^.h := dstheight;

    if SDL_MUSTLOCK(rz_src) then
      SDL_LockSurface(rz_src);

    if is32bit <> 0 then
    begin
      _zoomSurfaceRGBA(rz_src, rz_dst, flipx, flipy, smooth);
    end
    else
    begin
      {// Copy palette and colorkey info
      pal_src := SDL_GetSurfacePalette(rz_src);
      for i := 0 to pal_src^.ncolors - 1 do
        pal_dst^.colors[i] := pal_src^.colors[i];
      pal_dst^.ncolors := pal_src^.ncolors;

      // Call the 8bit transformation routine to do the zooming
      _zoomSurfaceY(rz_src, rz_dst, flipx, flipy); }
    end;

    if SDL_MUSTLOCK(rz_src) then
      SDL_UnlockSurface(rz_src);
  end;
  if src_converted <> 0 then
    SDL_DestroySurface(rz_src);

  Exit(rz_dst);
end;


end.
