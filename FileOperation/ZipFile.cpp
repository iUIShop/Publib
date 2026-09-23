#include "ZipFile.h"

#include <atlstr.h>
#include <strsafe.h>
// 解压Zip文件，如果lpszTargetUnZipFolder为NULL或空，则解压到zip文件所在文件夹
// 在Win7上验证成功，XP上未测试，有网友说不成功。
// 本方法使用API SHFileOperation解压缩，解压速度很慢。对于包含大量小文件的压缩包，例如包含4000多个文件，压缩后700MB左右的
// 压缩包，解压可能需要10多分钟。但同一个压缩包，换成7-zip解压，只需要30多秒。
int UnZipFile(LPCTSTR lpszSourceZipFile, LPCTSTR lpszTargetUnZipFolder)
{
	if (lpszSourceZipFile == NULL)
	{
		return -1;
	}

	SHFILEOPSTRUCT shFileOp;
	ZeroMemory(&shFileOp, sizeof(SHFILEOPSTRUCT));
	shFileOp.hwnd = NULL;
	shFileOp.wFunc = FO_COPY;

	//
	// pFrom中以NULL分隔多个Source File，最后一个文件以两个NULL结束，所以，
	// 如果直接把strSourceZipFile赋值给pFrom，将导致执行失败。 但类似_T("D:\\1.zip\\*.*")
	// 这种字符串直接赋值给pFrom是可以的。
	//
	CString strSourceZipFile = lpszSourceZipFile;
	if (strSourceZipFile.Right(4) != _T("\\*.*"))
	{
		strSourceZipFile += _T("\\*.*");
	}

	TCHAR szSourceZipFile[MAX_PATH * 4] = {0};
	StringCchCopy(szSourceZipFile, MAX_PATH * 4, strSourceZipFile);
	shFileOp.pFrom = szSourceZipFile;

	//
	// 如果目录文件夹为空，则释放到zip所在文件夹
	//
	CString strTargetFolder = lpszTargetUnZipFolder;
	if (strTargetFolder.IsEmpty())
	{
		strTargetFolder = lpszSourceZipFile;

		// 查找.zip，并把.zip后的字符删除
		strTargetFolder.MakeLower();
		strTargetFolder.MakeReverse();
		int nPos = strTargetFolder.Find(_T("piz."));
		strTargetFolder = lpszSourceZipFile;
		if (nPos >= 0)
		{
			strTargetFolder.Delete(strTargetFolder.GetLength() - nPos, nPos);

			// 删除文件名
			PathRemoveFileSpec(strTargetFolder.GetBuffer());
			strTargetFolder.ReleaseBuffer();
		}
	}

	// 如果目录不存在，SHFileOperation会自动创建多级目录
	shFileOp.pTo = (LPCTSTR)strTargetFolder;
	shFileOp.fFlags = FOF_NOCONFIRMMKDIR	// 不提示创建目录
		| FOF_NOCONFIRMATION				// 如果弹出任何对话框，自动回答Yes to All
		| FOF_SILENT;						// 不显示进度对话框
	int nRet = SHFileOperation(&shFileOp);

	return nRet;
}
