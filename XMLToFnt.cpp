/*
Original code by Stephane Cuillerdier (www.funparadigm.com)

Github of this project : https://github.com/aiekick/XMLToFnt

MIT License

Copyright (c) 2016 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <list>
#include <string>
using namespace std;

#include "tinyxml2.h"
using namespace tinyxml2;
/* Fnt File Format from http://www.angelcode.com/products/bmfont/doc/file_format.html

File tags
info

This tag holds information on how the font was generated.
face	This is the name of the true type font.
size	The size of the true type font.
bold	The font is bold.
italic	The font is italic.
charset	The name of the OEM charset used (when not unicode).
unicode	Set to 1 if it is the unicode charset.
stretchH	The font height stretch in percentage. 100% means no stretch.
smooth	Set to 1 if smoothing was turned on.
aa	The supersampling level used. 1 means no supersampling was used.
padding	The padding for each character (up, right, down, left).
spacing	The spacing for each character (horizontal, vertical).
outline	The outline thickness for the characters.
common

This tag holds information common to all characters.
lineHeight	This is the distance in pixels between each line of text.
base	The number of pixels from the absolute top of the line to the base of the characters.
scaleW	The width of the texture, normally used to scale the x pos of the character image.
scaleH	The height of the texture, normally used to scale the y pos of the character image.
pages	The number of texture pages included in the font.
packed	Set to 1 if the monochrome characters have been packed into each of the texture channels. In this case alphaChnl describes what is stored in each channel.
alphaChnl	Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
redChnl	Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
greenChnl	Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
blueChnl	Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
page

This tag gives the name of a texture file. There is one for each page in the font.
id	The page id.
file	The texture file name.
char

This tag describes on character in the font. There is one for each included character in the font.
id	The character id.
x	The left position of the character image in the texture.
y	The top position of the character image in the texture.
width	The width of the character image in the texture.
height	The height of the character image in the texture.
xoffset	How much the current position should be offset when copying the image from the texture to the screen.
yoffset	How much the current position should be offset when copying the image from the texture to the screen.
xadvance	How much the current position should be advanced after drawing the character.
page	The texture page where the character image is found.
chnl	The texture channel where the character image is found (1 = blue, 2 = green, 4 = red, 8 = alpha, 15 = all channels).
*/

template <typename T> string toStr(const T& t) { ostringstream os; os << t; return os.str(); } // sstream

static std::vector<std::string> splitString(const std::string& text, char delimiter)
{
	std::vector<std::string> result;

	std::string::size_type start = 0;
	std::string::size_type end = text.find(delimiter, start);

	while (end != string::npos)
	{
		std::string token = text.substr(start, end - start);

		result.push_back(token);

		start = end + 1;
		end = text.find(delimiter, start);
	}

	result.push_back(text.substr(start));

	return result;
}

static bool ReplaceString(std::string& str, const std::string& oldStr, const std::string& newStr)
{
	bool Finded = false;
	size_t pos = 0;
	while ((pos = str.find(oldStr, pos)) != std::string::npos) {
		Finded = true;
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return Finded;
}

/* 
cVariant for float, int and string
come from my Tools.hpp
*/
class cVariant
{
private:
	string type;
	string string_value;
	int int_value;
	float float_value;

public:
	cVariant() {}
	cVariant(int v) { int_value = v; type = "int"; }
	cVariant(float v) { float_value = v; type = "float"; }
	cVariant(string v) { string_value = v; type = "string"; }
	string getType(){return type;}
	float getF(){if (type == "string") return (float)atof(string_value.c_str());return float_value;}
	int getI(){if (type == "string") return atoi(string_value.c_str());return int_value;}
	string getS(){return string_value;}
};

/*
char id=48 x=2 y=2 width=63 height=63 xoffset=0 yoffset=0 xadvance=72 page=0 chnl=15
*/
struct FNT_Row_Struct
{
	string spriteName;
	int row; // row 0
	char id; // id=48
	int x; // x=2
	int y; //  y=2
	int w; // width=63
	int h; // height=63
	int xoff; // xoffset=0
	int yoff; // yoffset=0
	int xadv; // xadvance=72
	int page; // page=0
	int chnl; // chnl=15

	FNT_Row_Struct()
	{
		spriteName = "";
		row = 0;
		id = 0;
		x = 0;
		y = 0;
		w = 0;
		h = 0;
		xoff = 0;
		yoff = 0;
		xadv = 0;
		page = 0;
		chnl = 0;
	}
};

void ShowHelp()
{
	cout << "This app convert Xml file generated by TexturePacker (@CodeAndWeb) to font file (*.fnt)";
	cout << "The name of the sprite must be simple ascii char, except space char, if not the converting stop";
}

/* VARS */
list<FNT_Row_Struct> Rows;
string PictFilePathName;
int Width = 0;
int Height = 0;
int lineHeight = 0;

/* clean struct and map */
void Clean()
{
	Rows.clear();
	PictFilePathName = "";
	Width = 0;
	Height = 0;
	lineHeight = 0;
}

bool RecursParsingForLoading(tinyxml2::XMLElement* vElem)
{
	bool res = true;

	// The value of this child identifies the name of this element
	std::string strName = "";

	strName = vElem->Value(); // nom du block

	// <TextureAtlas imagePath="font_numbers_on.png" width="1741" height="1926">
	if (strName == "TextureAtlas")
	{
		for (const tinyxml2::XMLAttribute* attr = vElem->FirstAttribute(); attr != 0; attr = attr->Next())
		{
			std::string attName = attr->Name(); // nom de l'attribut
			cVariant attValue = attr->Value(); // valeur de l'attribut

			if (attName == "imagePath") PictFilePathName = attValue.getS();
			if (attName == "width") Width = attValue.getI();
			if (attName == "height") Height = attValue.getI();
		}
	}
	
	// <sprite n="a" x="488" y="643" w="416" h="640" pX="0.5" pY="0.5"/>
	if (strName == "sprite")
	{
		FNT_Row_Struct row;

		for (const tinyxml2::XMLAttribute* attr = vElem->FirstAttribute(); attr != 0; attr = attr->Next())
		{
			std::string attName = attr->Name(); // nom de l'attribut
			cVariant attValue = attr->Value(); // valeur de l'attribut

			if (attName == "n")
			{
				row.spriteName = attValue.getS();
				if (row.spriteName.size() == 1)
					row.id = int(attValue.getS()[0]);
				else if (row.spriteName == "space")
					row.id = int(' ');
			}
			if (attName == "x") row.x = attValue.getI();
			if (attName == "y") row.y = attValue.getI();
			if (attName == "w")
			{
				row.w = attValue.getI();
				row.xadv = row.w;
			}
			if (attName == "h")
			{
				row.h = attValue.getI();
				lineHeight += row.h;
			}
		}

		Rows.push_back(row);
	}

	// CHILDS // parse through all childs elements
	for (tinyxml2::XMLElement* child = vElem->FirstChildElement(); child != 0; child = child->NextSiblingElement())
	{
		if (RecursParsingForLoading(child->ToElement()) == false)
			return false; // si ya un probleme on sort tout de suite
	}

	return res;
}

void ConvertXmlFileToFnt(string vfaceName)
{
	Clean();

	string XMLFile_Src = vfaceName + ".xml";
	string FNTFile_Dst = vfaceName + ".fnt";

	// read xml file
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError err = doc.LoadFile(XMLFile_Src.c_str());
	if (err == tinyxml2::XMLError::XML_SUCCESS)
	{
		if (RecursParsingForLoading(doc.FirstChildElement("TextureAtlas")))
		{
			lineHeight = lineHeight * 1.0f / Rows.size();

			// write dst .fnt file
			ofstream wfntfile(FNTFile_Dst, ios::out | ios::trunc);
			if (wfntfile.is_open())
			{
				// fnt file format http://www.angelcode.com/products/bmfont/doc/file_format.html

				// we must write this :
				/*
				info face=PixelBlocFont size=72 bold=0 italic=0 charset= unicode= stretchH=100 smooth=1 aa=1 padding=2,2,2,2 spacing=0,0 outline=0
				common lineHeight=63 base=63 scaleW=392 scaleH=457 pages=1 packed=0
				page id=0 file="PixelBlocFont.png"
				chars count=36
				*/
				std::vector<std::string> splits = splitString(vfaceName, '\\');
				std::vector<std::string>::iterator it = splits.end(); it--; // last item
				wfntfile << "info face=" << (*it) << " size=72 bold=0 italic=0 charset= unicode= stretchH=100 smooth=1 aa=1 padding=2,2,2,2 spacing=0,0 outline=0" << endl;
				wfntfile << "common lineHeight=" << toStr(lineHeight) << " base=" << toStr(lineHeight) << " scaleW=" << toStr(Width) << " scaleH=" << toStr(Height) <<" pages=1 packed=0" << endl;
				wfntfile << "page id=0 file=\"" << PictFilePathName << "\"" << endl;
				wfntfile << "chars count=" << toStr(Rows.size()) << endl;

				for (list<FNT_Row_Struct>::iterator it = Rows.begin(); it != Rows.end(); ++it)
				{
					FNT_Row_Struct row = *it;

					// we must write :
					// char id=48 x=2 y=2 width=63 height=63 xoffset=0 yoffset=0 xadvance=72 page=0 chnl=15

					wfntfile
						<< "char id=" << toStr(int(row.id))
						<< " x=" << toStr(row.x)
						<< " y=" << toStr(row.y)
						<< " width=" << toStr(row.w)
						<< " height=" << toStr(row.h)
						<< " xoffset=" << toStr(row.xoff)
						<< " yoffset=" << toStr(row.yoff)
						<< " xadvance=" << toStr(row.xadv)
						<< " page=0 chnl=15" << endl;
				}
				
				wfntfile.close();
			}
			else
			{
				cout << "Unable to convert the XML file " << XMLFile_Src << " to Fnt File " << FNTFile_Dst << endl;
			}
		}
	}
}

int main(int argc, char *argv[]) // Don't forget first integral argument 'argc'
{
	// check if there is more than one argument and use the second one
	//  (the first argument is the executable)
	
	if (argc == 1)
		ShowHelp();

	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			string fileName = argv[i];
			if (ReplaceString(fileName, ".xml", ""))
			{
				ConvertXmlFileToFnt(fileName);
			}
			else
			{
				cout << "The file " << fileName << " is not a xml file" << endl;
			}
		}
	}

	return 0;
}


