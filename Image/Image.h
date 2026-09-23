#pragma once

// From zhipuqingyan
bool SaveHBITMAPToFile(HBITMAP hBitmap, const WCHAR* szFileName)
{
	// Retrieve the bitmap's dimensions, and calculate the size of
	// the .BMP file (including the header).
	BITMAP bmp;
	if (0 == GetObject(hBitmap, sizeof(BITMAP), &bmp))
	{
		return false;
	}

	BITMAPFILEHEADER bmfHeader = {0};
	BITMAPINFOHEADER bmiHeader = {0};

	// Initialize the fields in the BITMAPFILEHEADER structure.
	bmfHeader.bfType = 0x4D42; // BM
	bmfHeader.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp.bmWidthBytes * bmp.bmHeight);
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// Initialize the fields in the BITMAPINFOHEADER structure.
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biWidth = bmp.bmWidth;
	bmiHeader.biHeight = bmp.bmHeight;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = bmp.bmBitsPixel;
	bmiHeader.biCompression = BI_RGB;

	// Create the .BMP file.
	HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// Write the BITMAPFILEHEADER and BITMAPINFOHEADER structures to the .BMP file.
	DWORD dwWritten = 0;
	WriteFile(hFile, (LPVOID)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	WriteFile(hFile, (LPVOID)&bmiHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);

	// Calculate the size of the bitmap data (bytes per pixel * number of pixels).
	DWORD dwBytesPerRow = bmp.bmWidthBytes;
	DWORD dwTotalBytes = dwBytesPerRow * bmp.bmHeight;

	// Create a buffer to hold the bitmap data.
	BYTE* pBuffer = new BYTE[dwTotalBytes];

	// Retrieve the bitmap data into the buffer.
	GetDIBits(GetDC(NULL), hBitmap, 0, (UINT)bmp.bmHeight, pBuffer, (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS);

	// Write the bitmap data to the .BMP file.
	WriteFile(hFile, (LPVOID)pBuffer, dwTotalBytes, &dwWritten, NULL);

	// Clean up.
	delete[] pBuffer;
	CloseHandle(hFile);

	return true;
}
