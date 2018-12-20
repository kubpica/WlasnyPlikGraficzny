#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>

#endif
#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif
#include <math.h>
#define pi 3.14
#include <time.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <map>
using namespace std;

SDL_Surface *screen;
int width = 900;
int height = 600;
char const* tytul = "";


/* Info Header */
typedef struct PSKRINFOHEADER {
    char signature[2] = {'S','T'};
    //int dataOffset;
    int width;
    int height;
    int colorMode; //0=skala szarosci, 1=kolory narzucone, 2=paleta dedykowana
} PSKRINFOHEADER;

struct RGB {
    Uint8 r;
    Uint8 g;
    Uint8 b;
};

std::map<Uint8,RGB> paletaDedykowana;

void setPixel(int x, int y, Uint8 R, Uint8 G, Uint8 B);
SDL_Color getPixel (int x, int y);

void czyscEkran(Uint8 R, Uint8 G, Uint8 B);


void Funkcja1();
void Funkcja2();
/*void Funkcja3();
void Funkcja4();
void Funkcja5();
void Funkcja6();*/
void compress(FILE *in, FILE *out);
void decompress(FILE *in, FILE *out);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////               ////////////////////////////////////////////////////////////////////
///////////////////////////////////     KODER     ////////////////////////////////////////////////////////////////////
///////////////////////////////////               ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void dedicatedPalette(){
    struct RGBsum {
        unsigned long long r = 0;
        unsigned long long g = 0;
        unsigned long long b = 0;
        unsigned int howManyColors = 0;
    };
    std::map<Uint8,RGBsum> colors;

    for(int i=0; i<width; i++){
        for(int j=0; j<height; j++){
            SDL_Color color = getPixel(i, j);

            /* zamiana piksela z 24 na 5bit*/
            RGB c;
            c.r = color.r >> 6;
            c.g = color.g >> 6;
            c.b = color.b >> 7;

            Uint8 pixel = c.r | (c.g<<2) | (c.b<<4);
            if(colors.count(pixel) == 0){
                RGBsum newRGB;
                colors[pixel] = newRGB;
            }
            colors[pixel].r += color.r;
            colors[pixel].g += color.g;
            colors[pixel].b += color.b;
            colors[pixel].howManyColors++;
        }
    }

    std::map<Uint8,RGBsum>::iterator it = colors.begin();
    while (it != colors.end())
	{
		RGB color;
		color.r = it->second.r/it->second.howManyColors;
		color.g = it->second.g/it->second.howManyColors;
		color.b = it->second.b/it->second.howManyColors;
		paletaDedykowana[it->first] = color;
		it++;
	}
}

void FilterRoznicy(RGB & pixel, RGB upperPixel){ //symulacja filtra ró¿nicy dla kolorów
    int r = pixel.r - upperPixel.r;
    int g = pixel.g - upperPixel.g;
    int b = pixel.b - upperPixel.b;
    pixel.r = abs(r%256);
    pixel.g = abs(g%256);
    pixel.b = abs(b%256);
}

void zakoduj(ofstream* plik, int colorMode){
    for(int i=0; i<width; i++){
        for(int j=0; j<height; j++){
            RGB c;
            SDL_Color color = getPixel(i, j);
            c.r = color.r;
            c.g = color.g;
            c.b = color.b;

            /*Filtr ró¿nicy linii na kolorach przed kompresj¹ ????
            if(j!=0){
                SDL_Color upperColor = getPixel(i, j-1);
                RGB upperPixel;
                upperPixel.r = upperColor.r;
                upperPixel.g = upperColor.g;
                upperPixel.b = upperColor.b;
                FilterRoznicy(c, upperPixel);
            }*/

            /* zamiana piksela z 24 na 5bit*/
            Uint8 pixel;

            if(colorMode==0){
                //tryb skali szaroœci:
                pixel = 0.299*c.r+0.587*c.g+0.114*c.b;
                pixel = pixel >> 3; //5bitowa skala szaroœci
            }else{
                c.r = c.r >> 6;
                c.g = c.g >> 6;
                c.b = c.b >> 7;
                pixel = c.r | (c.g<<2) | (c.b<<4); //000BGGRR
            }
            plik->write(  reinterpret_cast < char *>( &pixel ),  sizeof(pixel));
        }
    }
    //cout << "temp: " << temp << endl;
    plik->flush();
    plik->close();
    //ofstream plik2("obrazek.pskr", std::ios::binary);
    FILE *out, *in;
    in = fopen("obrazek.pskr", "rb");
    out = fopen("obrazekCompr.pskr" , "wb");
    compress(in, out);
    fclose(in);
    fclose(out);
    SDL_Flip(screen);
    cout << "zakodowano" << endl;
}

void Funkcja1(int colorMode) {
    PSKRINFOHEADER header;
    header.width = width;
    header.height = height;
    header.colorMode = colorMode;

    ofstream plik ("obrazek.pskr", std::ios::binary);
    plik.write(reinterpret_cast < char *>( &header ), sizeof(header));

    if(colorMode == 2){
		paletaDedykowana.clear();
        dedicatedPalette();
        plik.write(reinterpret_cast < char *>( &paletaDedykowana ), sizeof(paletaDedykowana));
        //cout << "rozmiar mapy z paleta: " << sizeof(paletaDedykowana) << endl;
    }

    zakoduj(&plik, colorMode);
}

void Funkcja2(bool czySymulacjaPredyktora = false) {
    //ofstream plik("kolorDecompr.pskr", std::ios::binary);
    FILE *out, *in;
    out = fopen("obrazekDecompr.pskr", "wb");
    in = fopen("obrazekCompr.pskr" , "rb");
    decompress(in, out);
    fclose(in);
    fclose(out);
    ifstream plikde("obrazekDecompr.pskr", std::ios::binary);
    PSKRINFOHEADER header;
    plikde.read( reinterpret_cast < char *>( &header ), sizeof(header) );

    width = header.width;
    height = header.height;
    screen = SDL_SetVideoMode(width, height, 32,SDL_HWSURFACE|SDL_DOUBLEBUF);

    if(header.colorMode==2){ //wczytanie palety
        paletaDedykowana.clear();
        plikde.read( reinterpret_cast < char *>( &paletaDedykowana ), sizeof(paletaDedykowana) );
    }

    for(int i=0; i<width; i++){
        for(int j=0; j<height; j++){
            RGB color;
            //Zmienna pixel to jeden bajt odczytywany z pliku. Poszczególne sk³adowe zapisane s¹ na nastêpuj¹cych bitach: 000BGGRR
            RGB upperPixel;
            Uint8 pixel;
            plikde.read( reinterpret_cast < char *>( &pixel ), sizeof(pixel) );

            if(header.colorMode == 2){ //tryb kolorow dedykowanych:
                color = paletaDedykowana[pixel];
                setPixel(i, j, color.r, color.g, color.b);
            } else if(header.colorMode == 1){ //tryb kolorow narzuconych:

                //wydzielamy poszczególne sk³adowe:
                color.r = pixel & 0x03;
                color.g = (pixel & 0x0C) >> 2;
                color.b = (pixel & 0x10) >> 4;

                color.r *= 85; // R/3*255
                color.g *= 85;
                color.b *= 255;

                setPixel(i, j, color.r, color.g, color.b);
            } else{ //tryb stali szarosci:
                pixel = pixel/31.0*255.0;
                RGB szaryKolor;
                color.r = pixel;
                color.g = pixel;
                color.b = pixel;
            }
            if(j!=0 && czySymulacjaPredyktora) FilterRoznicy(color, upperPixel); //symulacja wygladu filtra ró¿nicy linii
            setPixel(i, j, color.r,color.g,color.b);
            upperPixel = color;
        }
    }

    SDL_Flip(screen);
}

void Funkcja3(){
    if(SDL_SaveBMP(screen, "obrazek.bmp")==-1)
        cout << "Blad! Nie udalo zapisac sie obrazek.bmp" << endl;
    else
        cout << "Pomyslnie zapisano wyswietlony obrazek do obrazek.bmp" << endl;
}

void compress(FILE *in, FILE *out)
{
   char cur, prev, tmp;
   unsigned char cnt = 0;
   int  cont;

   cont = fread(&cur, sizeof(char), 1, in);
   prev = cur + 1;
   while (cont == 1)
   {
      if (prev != cur)
      {
         if (cnt == 0)
         {
            /* znaki obok siebie rozne wrzuc do pliku wyjsciowego */
            fwrite(&cur, sizeof(char), 1, out);
         }
         else
         {
            /* skonczyla sie sekwencja powtarzajcych sie znakow */
            cnt--;
            fwrite(&prev, sizeof(char), 1, out);
            fwrite(&cnt, sizeof(char), 1, out);
            fwrite(&cur, sizeof(char), 1, out);
            cnt = 0;
         }
      }
      else
      {
         /* liczbe powtarzajacych sie znakow zapisujemy na jednym bajcie
            wypisz sekwencje jezeli ma ona maksymalna dlugosc */
         if (cnt == 255)
         {
            fwrite(&prev, sizeof(char), 1, out);
            fwrite(&cnt, sizeof(char), 1, out);
            cnt = 0;
            cont = fread(&cur, sizeof(char), 1, in);
            prev = cur + 1;
            continue;
         }
         else
         {
            /* licz powtarzajace sie znaki */
            cnt++;
         }
      }
      /* odczytaj kolejny znak z pliku wejsciowego */
      cont = fread(&tmp, sizeof(char), 1, in);
      if (cont == 1)
      {
         prev = cur;
         cur = tmp;
      }
   }

   /* jezeli plik konczy sie sekwencja wypisz ja */
   if (prev == cur)
   {
      cnt--;
      fwrite(&prev, sizeof(char), 1, out);
      fwrite(&cnt, sizeof(char), 1, out);
   }
}

/* dekompresja RLE */
void decompress(FILE *in, FILE *out)
{
   char cur, prev;
   unsigned char cnt = 0;
   int  cont, i;

   cont = fread(&cur, sizeof(char), 1, in);
   prev = cur + 1;
   while (cont == 1)
   {
      if (prev != cur)
      {
         /* znaki obok siebie rozne wrzuc do pliku wyjsciowego */
         fwrite(&cur, sizeof(char), 1, out);
         prev = cur;
         /* odczytaj kolejny znak */
         cont = fread(&cur, sizeof(char), 1, in);
      }
      else
      {
         /* znaki obok siebie sa rowne - mamy sekwencje */
         /* odczytaj dlugosc sekwencji i wrzuc ja do pliku wyjsciowego */
         cont = fread(&cnt, sizeof(char), 1, in);
         for (i=0; i<=cnt; i++)
         {
            fwrite(&cur, sizeof(char), 1, out);
         }
         /* odczytaj kolejny znak */
         cont = fread(&cur, sizeof(char), 1, in);
         prev = cur + 1;
      }
   }
}

void setPixel(int x, int y, Uint8 R, Uint8 G, Uint8 B)
{
  if ((x>=0) && (x<width) && (y>=0) && (y<height))
  {
    /* Zamieniamy poszczególne sk³adowe koloru na format koloru pixela */
    Uint32 pixel = SDL_MapRGB(screen->format, R, G, B);

    /* Pobieramy informacji ile bajtów zajmuje jeden pixel */
    int bpp = screen->format->BytesPerPixel;

    /* Obliczamy adres pixela */
    Uint8 *p = (Uint8 *)screen->pixels + y * screen->pitch + x * bpp;

    /* Ustawiamy wartoœæ pixela, w zale¿noœci od formatu powierzchni*/
    switch(bpp)
    {
        case 1: //8-bit
            *p = pixel;
            break;

        case 2: //16-bit
            *(Uint16 *)p = pixel;
            break;

        case 3: //24-bit
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;

        case 4: //32-bit
            *(Uint32 *)p = pixel;
            break;

    }
         /* update the screen (aka double buffering) */
  }
}

void ladujBMP(char const* nazwa, int x, int y)
{
    SDL_Surface* bmp = SDL_LoadBMP(nazwa);
    if (!bmp)
    {
        printf("Unable to load bitmap: %s\n", SDL_GetError());
    }
    else
    {
        width = bmp->w;
        height = bmp->h;
        screen = SDL_SetVideoMode(width, height, 32,SDL_HWSURFACE|SDL_DOUBLEBUF);
        SDL_Rect dstrect;
        dstrect.x = x;
        dstrect.y = y;
        SDL_BlitSurface(bmp, 0, screen, &dstrect);
        SDL_Flip(screen);
        SDL_FreeSurface(bmp);
    }

}

void czyscEkran(Uint8 R, Uint8 G, Uint8 B)
{
    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, R, G, B));
    SDL_Flip(screen);

}

SDL_Color getPixel (int x, int y) {
    SDL_Color color ;
    Uint32 col = 0 ;
    if ((x>=0) && (x<width) && (y>=0) && (y<height)) {
        //determine position
        char* pPosition=(char*)screen->pixels ;
        //offset by y
        pPosition+=(screen->pitch*y) ;
        //offset by x
        pPosition+=(screen->format->BytesPerPixel*x);
        //copy pixel data
        memcpy(&col, pPosition, screen->format->BytesPerPixel);
        //convert color
        SDL_GetRGB(col, screen->format, &color.r, &color.g, &color.b);
    }
    return ( color ) ;
}


int main ( int argc, char** argv )
{
    // console output
    freopen( "CON", "wt", stdout );
    freopen( "CON", "wt", stderr );

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    screen = SDL_SetVideoMode(width, height, 32,SDL_HWSURFACE|SDL_DOUBLEBUF);
    //screen=SDL_SetVideoMode(width, height, 8, SDL_HWPALETTE);
    if ( !screen )
    {
        printf("Unable to set video: %s\n", SDL_GetError());
        return 1;
    }

    SDL_WM_SetCaption( tytul , NULL );
    // program main loop
        /*cout<< "1. Zakodowanie obrazka do .pskr (5 bit, RLE) z wykorzystaniem 32 narzuconych barw"<< endl;
        cout<< "2. Zakodowanie obrazka do .pskr (5 bit, RLE) z wykorzystaniem 32 dedykowanych barw"<< endl;
        cout<< "3. Zakodowanie obrazka do .pskr (5 bit, RLE) w 32 stopniowej skali szarosci"<< endl;
        cout<< "4. Zdekodowanie i wyswietlenie obrazka"<< endl;*/
        cout<< "a,s,d,f - wczytaj obrazek1,2,3,4.bmp" << endl;
        cout<< "Funckje zapisu do obrazek.pskr:" << endl;
        cout<< "1. Zakoduj wyswietlony obrazek z narzuconymi kolorami"<< endl;
        cout<< "2. Zakoduj wyswietlony obrazek z dedykowana paleta"<< endl;
        cout<< "3. Zakoduj wyswietlony obrazek w skali szarosci"<< endl;
        cout<< "Funkcje wczytywania obrazek.pskr:" << endl;
        cout<< "4. Wyswietl zakodowany obrazek (w normalny sposob)"<< endl;
        cout<< "5. Wyswietl zakodowany obrazek symulujac wyglad po zastosowaniu filtra roznicy linii"<< endl;
        cout<< "Funkcja zapisu do obrazek.bmp:" << endl;
        cout<< "6. Zapisz wyswietlony obrazek do obrazek.bmp"<< endl;
    bool done = false;
    while (!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {

            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
                {
                    // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        done = true;
                    if (event.key.keysym.sym == SDLK_1)
                        Funkcja1(1);
                    if (event.key.keysym.sym == SDLK_2)
                        Funkcja1(2);
                    if (event.key.keysym.sym == SDLK_3)
                        Funkcja1(0);
                    if (event.key.keysym.sym == SDLK_4)
                        Funkcja2(false);
                    if (event.key.keysym.sym == SDLK_5)
                        Funkcja2(true);
                    if (event.key.keysym.sym == SDLK_6)
                        Funkcja3();
                    if (event.key.keysym.sym == SDLK_a)
                        ladujBMP("obrazek1.bmp", 0, 0);
                    if (event.key.keysym.sym == SDLK_s)
                        ladujBMP("obrazek2.bmp", 0, 0);
                    if (event.key.keysym.sym == SDLK_d)
                        ladujBMP("obrazek3.bmp", 0, 0);
                    if (event.key.keysym.sym == SDLK_f)
                        ladujBMP("obrazek4.bmp", 0, 0);
                    if (event.key.keysym.sym == SDLK_b)
                        czyscEkran(0, 0, 0);          break;
                     }
            } // end switch
        } // end of message processing

    } // end main loop


    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}
