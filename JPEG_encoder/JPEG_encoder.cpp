// JPEG_encoder.cpp : 定義主控台應用程式的進入點。
//

#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <stdlib.h>

using namespace std;
using namespace cv;

/*離散餘旋轉換之係數*/
float C(int u);

/*離散餘旋轉換*/
vector<vector<float>> DCT(vector<vector<int>> f);

/*灰階值量化*/
vector<vector<int>> Quantize(vector<vector<float>> F);

/*Zigzag掃描排序*/
vector<int> ZigZag(vector<vector<int>> QF);

/*DC及AC係數查表*/
int getCat(int a);

/*找尋最後一個不為零的索引值(若皆為零則回傳-1)*/
int LastNonZero(vector<int> ZZ);

/*十進位轉二進位*/
string ten2two(int a10);

/*負數十進位轉二進位的補數*/
string negten2two(int a10);

/*對DC進行熵編碼*/
string getDCcode(int a);

/*對AC進行熵編碼*/
string getACcode(vector<int> ZZ);

/*統整上述步驟*/
string Encode(vector<vector<int>> f, int DCold, int &DCnew);

/*字節填充(用以防止編碼被判識為標記)*/
void bytestuffing(string &writecode);

/*二進位轉十進位*/
int two2ten(string a2);

/*將string內的位元每8個為一單位轉換為十進位的string*/
vector<int> change(string writecode);

/*十六進位轉十進位*/
int hex2dec(string input);

/*將尺寸以轉十六進位顯示*/
void xy2hex(int inX, int &outX1, int &outX2);

int main()
{
	string fname;
	cout << "\n\nEnter name of image file : ";
	cin >> fname;
	//fname = "C:\\Users\\Jimmy\\Desktop\\test\\test.bmp";
	Mat image = imread(fname,0);

	int maxX = image.cols;
	int maxY = image.rows;

	Mat padded;

	int xnum = maxX/8;
	int ynum = maxY/8;

	int addx = 0, addy = 0;

	if (maxX % 8 != 0)
		addx = (xnum + 1) * 8 - maxX;

	if (maxY % 8 != 0)
		addy = (ynum + 1) * 8 - maxY;


	copyMakeBorder(image, padded, 0, addy, 0, addx, BORDER_CONSTANT, Scalar::all(0));

	int newX = padded.cols;
	int newY = padded.rows;

	//Mat YCrCbimage;
	//cvtColor(padded, YCrCbimage, COLOR_BGR2YCrCb);

	//vector<Mat> splitimage(3);
	//split(YCrCbimage, splitimage);

	//Mat Yimage = splitimage[0];

	Mat Yimage = image;

	vector<vector<int>> intensity;
	for (int i = 0; i < newY; i++)
	{
		intensity.push_back(vector<int>());
		for (int j = 0; j < newX; j++)
		{
			intensity[i].push_back((int)Yimage.at<uchar>(i, j));
		}
	}
	cout << "Enter output file name : ";
	//fname = "C:\\Users\\Jimmy\\Desktop\\test\\test.jpg";
	cin >> fname;

	fstream fout(fname, ios::out | ios::binary);

	int DCold = 0;
	int DCnew = 0;

	string writecode;

	for (int i = 0; i < newY / 8; i++)
	{
		for (int j = 0; j < newX / 8; j++)
		{
			vector<vector<int>> f;
			string code;
			for (int i1 = 0; i1 < 8; i1++)
			{
				f.push_back(vector<int>());
				for (int j1 = 0; j1 < 8; j1++)
				{
					if ((i * 8 + i1 < newY) && (j * 8 + j1 < newX))
						f[i1].push_back(intensity[i * 8 + i1][j * 8 + j1] - 128);
					else
						f[i1].push_back(0);
				}
			}
			code = Encode(f, DCold, DCnew);
			
			DCold = DCnew;
			writecode += code;
		}
	}

	bytestuffing(writecode);
	vector<int> decimalcode = change(writecode);

	//圖像結束
	decimalcode.push_back(255);
	decimalcode.push_back(217);

	int hexx1, hexx2, hexy1, hexy2;

	xy2hex(maxX, hexx1, hexx2);
	xy2hex(maxY, hexy1, hexy2);

	/*文件標記碼*/
	vector<int> signal = {
		//圖像開始(FFD8)
		255,216,

		//應用程序保留標記0(FFE0)
		255,224,0,16,74,70,73,70,0,1,1,0,0,1,0,1,0,0,

		//定義亮度量化表(FFDB)
		255,219,0,67,0,16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,
		24,49,36,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,
		109,81,87,95,98,103,194,103,62,77,113,121,112,100,120,92,101,103,99,
		
		//幀圖像開始(FFC0)
		255,192,0,11,8,hexy1,hexy2,hexx1,hexx2,1,1,17,0,

		//定義DCHuffman表(FFC4)
		255,196,0,31,0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,

		//定義ACHuffman表(FFC4)
		255,196,0,181,16,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125,1,2,3,0,4,17,5,18,33,49,
		65,6,19,81,97,7,34,113,20,50,129,145,161,8,35,66,177,193,21,82,209,240,36,51,
		98,114,130,9,10,22,23,24,25,26,37,38,39,40,41,42,52,53,54,55,56,57,58,67,68,
		69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,106,115,
		116,117,118,119,120,121,122,131,132,133,134,135,136,137,138,146,147,148,149,150,
		151,152,153,154,162,163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,
		185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,216,217,218,
		225,226,227,228,229,230,231,232,233,234,241,242,243,244,245,246,247,248,249,250,
		
		//掃描開始(FFDA)
		255,218,0,8,1,1,0,0,63,0
	};



	for (int i = 0; i < signal.size(); ++i)
	{
		int temp= signal[i];
		char str=temp;
		fout.write((char *)&str, sizeof(str));
	}

	for (int i = 0; i < decimalcode.size(); ++i)
	{
		int temp = decimalcode[i];
		char str = temp;
		fout.write((char *)&str, sizeof(str));
	}

	fout.close();

	return 0;
}


/*離散餘旋轉換之係數*/
float C(int u)
{
	if (u == 0)
		return (1.0 / sqrt(2.0));
	else
		return 1.0;
}

/*離散餘旋轉換*/
vector<vector<float>> DCT(vector<vector<int>> f)
{
	float a;
	vector<vector<float>> F;
	for (int u = 0; u < 8; u++)
	{
		F.push_back(vector<float>());
		for (int v = 0; v < 8; v++)
		{
			a = 0.0;
			for (int x = 0; x < 8; x++)
				for (int y = 0; y < 8; y++)
					a += float(f[x][y])*cos((2.0*float(x) + 1.0)*float(u)*3.14159265359 / 16.0)*cos((2.0*float(y) + 1.0)*float(v)*3.14159265359 / 16.0);
			F[u].push_back(float(0.25*C(u)*C(v)*a));
		}
	}
	return F;
}

/*灰階值量化*/
vector<vector<int>> Quantize(vector<vector<float>> F)
{
	vector<vector<float>> q = { {16,11,10,16,24,40,51,61},
		{12,12,14,19,26,58,60,55 },
		{14,13,16,24,40,57,69,56},
		{14,17,22,29,51,87,80,62},
		{18,22,37,56,68,109,103,77},
		{24,35,55,64,81,104,113,92},
		{49,64,78,87,103,121,120,101},
		{72,92,95,98,112,100,103,99} };

	vector<vector<int>> QF;
	for (int i = 0; i < 8; i++)
	{
		QF.push_back(vector<int>());
		for (int j = 0; j < 8; j++)
		{
			QF[i].push_back((int)(F[i][j] / q[i][j]));
		}
	}
	return QF;
}

/*Zigzag掃描排序*/
vector<int> ZigZag(vector<vector<int>> QF)
{
	int i = 0, j = 0, k = 0, d = 0;
	vector<int> ZZ;
	while (k < 36)
	{
		k++;
		ZZ.push_back(QF[i][j]);
		if ((i == 0) && (j % 2 == 0))
		{
			j++;
			d = 1;
		}
		else if ((j == 0) && (i % 2 == 1))
		{
			i++;
			d = 0;
		}
		else if (d == 0)
		{
			i--;
			j++;
		}
		else
		{
			i++;
			j--;
		}
	}
	i = 7;
	j = 1;
	d = 0;
	while (k < 64)
	{
		k++;
		ZZ.push_back(QF[i][j]);
		if ((i == 7) && (j % 2 == 0))
		{
			j++;
			d = 0;
		}
		else if ((j == 7) && (i % 2 == 1))
		{
			i++;
			d = 1;
		}
		else if (d == 0)
		{
			i--;
			j++;
		}
		else
		{
			i++;
			j--;
		}
	}
	return ZZ;
}

/*DC及AC係數查表*/
int getCat(int a)
{
	if (a == 0)
		return 0;
	else if (abs(a) <= 1)
		return 1;
	else if (abs(a) <= 3)
		return 2;
	else if (abs(a) <= 7)
		return 3;
	else if (abs(a) <= 15)
		return 4;
	else if (abs(a) <= 31)
		return 5;
	else if (abs(a) <= 63)
		return 6;
	else if (abs(a) <= 127)
		return 7;
	else if (abs(a) <= 255)
		return 8;
	else if (abs(a) <= 511)
		return 9;
	else if (abs(a) <= 1023)
		return 10;
	else if (abs(a) <= 2047)
		return 11;
	else if (abs(a) <= 4095)
		return 12;
	else if (abs(a) <= 8191)
		return 13;
	else if (abs(a) <= 16383)
		return 14;
	else
		return 15;
}

/*找尋最後一個不為零的索引值(若皆為零則回傳-1)*/
int LastNonZero(vector<int> ZZ)
{
	int lastindex = -1;
	for (int i = 0; i < ZZ.size(); ++i)
	{
		if (ZZ[i] != 0)
		{
			lastindex = i;
		}
	}
	return lastindex;
}

/*十進位轉二進位*/
string ten2two(int a10)
{
	int n = a10;
	vector<int> a2_temp;
	while (n > 0)
	{
		int k = n % 2;
		n /= 2;
		a2_temp.push_back(k);
	}

	string a2;
	for (int i = a2_temp.size() - 1; i >= 0; i--)
	{
		a2 += to_string(a2_temp[i]);
	}
	return a2;
}

/*負數十進位轉二進位的補數*/
string negten2two(int a10)
{
	int n = abs(a10);
	vector<int> a2_temp;
	while (n > 0)
	{
		int k = n % 2;
		n /= 2;
		if (k == 0)
			k = 1;
		else
			k = 0;
		a2_temp.push_back(k);
	}

	string a2;
	for (int i = a2_temp.size() - 1; i >= 0; i--)
	{
		a2 += to_string(a2_temp[i]);
	}
	return a2;
}

/*對DC進行熵編碼*/
string getDCcode(int a)
{
	vector<string> code = { "00","010","011","100","101","110","1110","11110","111110","1111110","11111110","111111110" };
	string DCcode;
	int cat = getCat(a);
	DCcode = code[cat];
	if (a == 0)
	{
		return DCcode;
	}
	else if (a > 0)
	{
		DCcode += ten2two(a);
		return DCcode;
	}
	else if (a < 0)
	{
		DCcode += negten2two(a);
		return DCcode;
	}
}

/*對AC進行熵編碼*/
string getACcode(vector<int> ZZ)
{
	string EOB = "1010";
	string ZRL = "11111111001";
	string ACcode;
	vector<vector<string>> code = {
		{ "00","01","100","1011","11010","1111000","11111000","1111110110","1111111110000010","1111111110000011"},
		{ "1100","11011","1111001","111110110","11111110110","1111111110000100","1111111110000101","1111111110000110","1111111110000111","1111111110001000"},
		{ "11100","11111001","1111110111","111111110100","1111111110001001","1111111110001010","1111111110001011","1111111110001100","1111111110001101","1111111110001110"},
		{ "111010","111110111","111111110101","1111111110001111","1111111110010000","1111111110010001","1111111110010010","1111111110010011","1111111110010100","1111111110010101"},
		{ "111011","1111111000","1111111110010110","1111111110010111","1111111110011000","1111111110011001","1111111110011010","1111111110011011","1111111110011100","1111111110011101"},
		{ "1111010","11111110111","1111111110011110","1111111110011111","1111111110100000","1111111110100001","1111111110100010","1111111110100011","1111111110100100","1111111110100101"},
		{ "1111011","111111110110","1111111110100110","1111111110100111","1111111110101000","1111111110101001","1111111110101010","1111111110101011","1111111110101100","1111111110101101"},
		{ "11111010","111111110111","1111111110101110","1111111110101111","1111111110110000","1111111110110001","1111111110110010","1111111110110011","1111111110110100","1111111110110101"},
		{ "111111000","111111111000000","1111111110110110","1111111110110111","1111111110111000","1111111110111001","1111111110111010","1111111110111011","1111111110111100","1111111110111101"},
		{ "111111001","1111111110111110","1111111110111111","1111111111000000","1111111111000001","1111111111000010","1111111111000011","1111111111000100","1111111111000101","1111111111000110"},
		{ "111111010","1111111111000111","1111111111001000","1111111111001001","1111111111001010","1111111111001011","1111111111001100","1111111111001101","1111111111001110","1111111111001111"},
		{ "1111111001","1111111111010000","1111111111010001","1111111111010010","1111111111010011","1111111111010100","1111111111010101","1111111111010110","1111111111010111","1111111111011000"},
		{ "1111111010","1111111111011001","1111111111011010","1111111111011011","1111111111011100","1111111111011101","1111111111011110","1111111111011111","1111111111100000","1111111111100001"},
		{ "11111111000","1111111111100010","1111111111100011","1111111111100100","1111111111100101","1111111111100110","1111111111100111","1111111111101000","1111111111101001","1111111111101010"},
		{ "1111111111101011","1111111111101100","1111111111101101","1111111111101110","1111111111101111","1111111111110000","1111111111110001","1111111111110010","1111111111110011","1111111111110100"},
		{ "1111111111110101","1111111111110110","1111111111110111","1111111111111000","1111111111111001","1111111111111010","1111111111111011","1111111111111100","1111111111111101","1111111111111110"}

	};

	int K = LastNonZero(ZZ);

	if (K < 1)				//若所有AC皆為零(索引值1到63)
	{
		ACcode = EOB;
		return ACcode;
	}

	int nonzero = 0;

	for (int i = 1; i <= K; ++i)
	{
		int B = ZZ[i];
		if (B > 0)
		{
			int index = getCat(B);
			ACcode += code[nonzero][index-1] + ten2two(B);
			nonzero = 0;
		}
		else if (B < 0)
		{
			int index = getCat(B);
			ACcode += code[nonzero][index-1] + negten2two(B);
			nonzero = 0;
		}
		else
		{
			if (nonzero == 15)
			{
				ACcode += ZRL;
				nonzero = 0;
			}
			else
				nonzero = nonzero + 1;
		}

	}
	ACcode += EOB;

	return ACcode;
}

/*統整上述步驟*/
string Encode(vector<vector<int>> f, int DCold, int &DCnew)
{
	vector<vector<float>> F = DCT(f);
	vector<vector<int>> QF = Quantize(F);

	DCnew = QF[0][0];
	QF[0][0] -= DCold;

	vector<int> ZZ = ZigZag(QF);

	string code = getDCcode(ZZ[0]);
	code += getACcode(ZZ);

	return code;
}

/*字節填充(用以防止編碼被判識為標記)*/
void bytestuffing(string &writecode)
{
	int addbytenum = 0;
	int oldsize = writecode.size();
	for (int i = 0; i < writecode.size(); i = i + 8)
	{
		string bit_val (writecode, i,8);
		if (bit_val == "11111111")
		{
			writecode.insert(i+8, "00000000");
			i = i + 8;
		}
	}
}

/*二進位轉十進位*/
int two2ten(string a2)
{
	int a10 = 0;
	for (int i = 0; i < a2.size(); ++i)
	{
		if (a2[i] == '1')
		{
			a10 += pow(2, 7 - i);
		}
	}
	return a10;
}

/*將string內的位元每8個為一單位轉換為十進位的string*/
vector<int> change(string writecode)
{
	int numbytes = writecode.size() / 8;
	int diff_writecode = writecode.size() % 8;
	vector<int> outputcode;
	for (int i = 0; i < numbytes * 8; i = i + 8)
	{
		string str(writecode, i, 8);
		outputcode.push_back(two2ten(str));
	}

	if (diff_writecode != 0)
	{
		string str1(8 - diff_writecode, 0);
		string str2(writecode, numbytes * 8, diff_writecode);
		str2 += str1;
		outputcode.push_back(two2ten(str2));
	}
	return outputcode;
}

/*十六進位轉十進位*/
int hex2dec(string input)
{
	unsigned long long n;
	stringstream ss;
	ss << hex << uppercase << input;
	//ss << hex << lowercase << input;
	ss >> n;
	int output = n;
	return output;
}

/*將尺寸以轉十六進位顯示*/
void xy2hex(int inX, int &outX1, int &outX2)
{
	char x_temp[5];

	_itoa_s(inX, x_temp, 5, 16);

	string x_temptemp(x_temp, 0, 4);

	string x_temp1;
	string x_temp2;

	if (x_temptemp.size() == 1)
	{
		x_temp1.push_back('0');
		x_temp1.push_back('0');
		x_temp2.push_back('0');
		x_temp2.push_back(x_temp[0]);
	}
	else if (x_temptemp.size() == 2)
	{
		x_temp1.push_back('0');
		x_temp1.push_back('0');
		x_temp2.push_back(x_temp[0]);
		x_temp2.push_back(x_temp[1]);
	}
	else if (x_temptemp.size() == 3)
	{
		x_temp1.push_back('0');
		x_temp1.push_back(x_temp[0]);
		x_temp2.push_back(x_temp[1]);
		x_temp2.push_back(x_temp[2]);
	}
	else
	{
		x_temp1.push_back(x_temp[0]);
		x_temp1.push_back(x_temp[1]);
		x_temp2.push_back(x_temp[2]);
		x_temp2.push_back(x_temp[3]);
	}

	/*十六進位轉十進位*/
	outX1 = hex2dec(x_temp1);
	outX2 = hex2dec(x_temp2);
}






