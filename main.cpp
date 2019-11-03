#include <windows.h>  // for MS Windows
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include <stdlib.h>


//#include "stdafx.h"

#define N 70 //Numero de particulas
#define dt 0.03 // Delta T
#define m 0.3 //massa de cada célula
#define K 100 //Constante de elasticidade
#define Fat 0.05 // Constante de atrito
#define g coord(0,0,10) // Aceleracao da gravidade
#define H 200 //Altura H
#define T 1
#define R 10 //raio da bolinha

int especMaterial; // para reflexão
double t; // para contagem do numero de iteracoes
bool contato; // para marcar quando o tecido toca a esfera

class coord{
	public:
	double x,y,z;
	coord(){
		x=0;
		y=0;
		z=0;
	}

	coord(double x,double y, double z){
		this->x=x;
		this->y=y;
		this->z=z;
	}

	coord& operator = (const coord& a)
	{
		this->x = a.x;
		this->y = a.y;
		this->z = a.z;
		return *this;
	}
};

coord calcDistancia(coord i, coord j){ // calcula a distancia entre duas particulas
	coord ret;
	ret.x = j.x-i.x;
	ret.y = j.y-i.y;
	ret.z = j.z-i.z;
	return ret;
}

coord operator+(const coord &i, const coord &j){ // suma duas forças (particulas)
	coord ret;
	ret.x=i.x+j.x;
	ret.y=i.y+j.y;
	ret.z=i.z+j.z;
	return ret;
}

coord operator-(const coord &i, const coord &j){ // subtrai duas forças (particulas)
	coord ret;
	ret.x=i.x-j.x;
	ret.y=i.y-j.y;
	ret.z=i.z-j.z;
	return ret;
}


coord operator-(const coord &i){ // define operador - (menos) para uma particula (forca)
	coord ret;
	ret.x=-i.x;
	ret.y=-i.y;
	ret.z=-i.z;
	return ret;
}

coord operator*(const coord &i, double f){ // define um operador * (multiplicação) p/ uma particula
	coord ret;
	ret.x = i.x*f;
	ret.y = i.y*f;
	ret.z = i.z*f;
	return ret;
}

coord operator/(const coord &i, double f){ // define operador divisão para uma particula
	coord ret;
	ret.x = i.x/f;
	ret.y = i.y/f;
	ret.z = i.z/f;
	return ret;
}

coord Fatrito[N][N];	// Força de Atrito
coord Felastica[N][N];	// Força Elástica
coord quant_movimento; // Energia Cinética
coord A[N][N];	// Aceleração
coord V[N][N];	// Velocidade
coord P[N][N];	// posição



// Função callback chamada quando o tamanho da janela é alterado
void AlteraTamanhoJanela(GLsizei w, GLsizei h){
	GLsizei largura, altura;

	// Evita a divisao por zero
	if(h == 0) h = 1;

	// Atualiza as variáveis
	largura = w;
	altura = h;

	// Especifica as dimensões da Viewport
	glViewport(0, 0, largura, altura);

	// Inicializa o sistema de coordenadas
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,w/h,0.5,500);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// Função responsável pela especificação dos parâmetros de iluminação
void DefineIluminacao (void){
	GLfloat MatDifuseFront[4]={1.0,0.0,0.0,1.0};
	GLfloat MatDifuseBack[4] ={0.0,0.0,1.0,1.0};

	GLfloat luzAmbiente[4]={0.2,0.2,0.2,1.0};
	GLfloat luzDifusa[4]={0.7,0.7,0.7,1.0};	// "cor"
	GLfloat luzEspecular[4]={0.5, 0.5, 0.5, 1.0};// "brilho"
	GLfloat posicaoLuz[4]={0, 250, 0, 1};
	GLfloat posicaoLuz2[4]={0, -250, 0, 1};

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	// Capacidade de brilho do material
	GLfloat especularidade[4]={1.0,1.0,1.0,1.0};
	GLint especMaterial = 5;
	//GLfloat difusa[4]={1.0,0,0,1.0};

	// Define a refletância do material
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR, especularidade);
	// Define a concentração do brilho
	glMateriali(GL_FRONT_AND_BACK,GL_SHININESS,especMaterial);
	glMaterialfv(GL_FRONT,GL_DIFFUSE,MatDifuseFront);
	glMaterialfv(GL_BACK,GL_DIFFUSE,MatDifuseBack);

	// Ativa o uso da luz ambiente
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmbiente);

	// Define os parâmetros da luz de número 0
	glLightfv(GL_LIGHT0, GL_AMBIENT, luzAmbiente);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, luzDifusa );
	glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular );
	glLightfv(GL_LIGHT0, GL_POSITION, posicaoLuz );

	glLightfv(GL_LIGHT1, GL_AMBIENT, luzAmbiente);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, luzDifusa );
	glLightfv(GL_LIGHT1, GL_SPECULAR, luzEspecular );
	glLightfv(GL_LIGHT1, GL_POSITION, posicaoLuz2 );

	//Habilita o uso de iluminação
	glEnable(GL_LIGHTING);
	// Habilita a luz de número 0
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	// Habilita o depth-buffering
	glEnable(GL_DEPTH_TEST);
}


double modulo(const coord &i){ // define operador modulo p/ particula
	return sqrt(i.x*i.x + i.y*i.y+i.z*i.z);
}

void imprime(const coord &i){ // define operador imprime p/ particula
	printf("x: %f\ty: %f\tz:%f\n",i.x,i.y,i.z) ;
}

void setMesh(){ // essa função inicializa a malha e as suas propriedades fisicas
	for (int i=0;i<N;i++){
		for (int j=0;j<N;j++){
		A[i][j].x = 0;
		A[i][j].y = 0;
		A[i][j].z = 0;
		V[i][j].x = 0;
		V[i][j].y = 0;
		V[i][j].z = 0;
		Felastica[i][j].x = 0;
		Felastica[i][j].y = 0;
		Felastica[i][j].z = 0;
		Fatrito[i][j].x =  0;
		Fatrito[i][j].y =   0;
		Fatrito[i][j].z =  0;
		}
	}
	for (int i=0;i<N;i++){ // este trecho inicializa posição
		for (int j=0;j<N;j++){
		P[i][j].x = i; ;
		P[i][j].y = j; ;
		P[i][j].z = H;
		}
	}
}

void calcHook(){ // esta funcao calcula a forca elastica entre as particulas vizinhas
	coord dist;
	for (int i=0;i<N;i++){
		for (int j=0;j<N;j++){
			Felastica[i][j] = coord(0,0,0);
			if(i>0){
				Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i-1][j])*(modulo(calcDistancia(P[i][j],P[i-1][j]))- 1.0);
				if(j>0){
					Felastica[i][j] = Felastica[i][j]+calcDistancia(P[i][j],P[i-1][j-1])*(modulo(calcDistancia(P[i][j],P[i-1][j- 1]))-sqrt(2.0));
				}
				if(j<N-1){
					Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i-1][j+1])*(modulo(calcDistancia(P[i][j],P[i-1][j+1]))-sqrt(2.0));
				}
			}

			if(i<N-1){
				Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i+1][j])*(modulo(calcDistancia(P[i][j],P[i+1][j]))-1.0);
				if(j>0){
					Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i+1][j-1])*(modulo(calcDistancia(P[i][j],P[i+1][j-1]))-sqrt(2.0));
				}
				if(j<N-1){
					Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i+1][j+1])*(modulo(calcDistancia(P[i][j],P[i+1][j+1]))-sqrt(2.0));
				}
			}
			if(j>0){
				Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i][j-1])*(modulo(calcDistancia(P[i][j],P[i][j-1]))-1.0);
			}
			if(j<N-1){
				Felastica[i][j] = Felastica[i][j] + calcDistancia(P[i][j],P[i][j+1])*(modulo(calcDistancia(P[i][j],P[i][j+1]))-1.0);
			}

			Felastica[i][j] = Felastica[i][j] * K;
		}
	}
}

void calcFat(){ // funcao para calcular a força de atrito
	for (int i=0;i<N;i++){
		for (int j=0;j<N;j++){
			Fatrito[i][j]=coord(0,0,0);

			if(i>0){
				if(j>0){
					Fatrito[i][j] = Fatrito[i][j]+V[i-1][j-1];
				}
				Fatrito[i][j] = Fatrito[i][j]+V[i-1][j];

				if(j<N-1){
					Fatrito[i][j] = Fatrito[i][j]+V[i-1][j+1];
				}
			}

			if(j>0){
				Fatrito[i][j] = Fatrito[i][j]+V[i][j-1];
			}

			if(j<N-1){
				Fatrito[i][j] = Fatrito[i][j]+V[i][j+1];
			}

			if(i<N-1){

				if(j>0){
					Fatrito[i][j] = Fatrito[i][j]+V[i+1][j-1];
				}

				Fatrito[i][j] = Fatrito[i][j]+V[i+1][j];

				if(j<N-1){
					Fatrito[i][j] = Fatrito[i][j]+V[i+1][j+1];
				}
			}

			Fatrito[i][j] = Fatrito[i][j]*Fat;
		}
	}
}

void calcPosicoes(){ // funcção para calcular as posicoes das particulas
	calcHook(); // calcula forca elastica
	calcFat();	// calcula forca de atrito

	double M1;

	for (int i=0;i<N-1;i++){
		for (int j=0;j<N;j++){
			if(i<N/2){
				M1 =m;
			}else{
				M1=m;
			}
			// calcula a aceleração
			A[i][j] = ((-g*M1 + Felastica[i][j] - Fatrito[i][j])/2 / M1);
		}
	}
	// calcula a energia cinetica
	quant_movimento=coord(0,0,0);

	// controla a energia cinetica pra fazer a simulacao parar
	for (int i=0;i<N-1;i++){
		for (int j=0;j<N;j++){
			V[i][j]= V[i][j]+ A[i][j]*dt;
			if( modulo(P[i][j]-coord(N/2,N/2,H*0.8))>R+1 ){
				P[i][j] = P[i][j] + V[i][j]*dt;
				quant_movimento=quant_movimento+V[i][j]; // acumula energia cinetica (velocidade)
			}else{
				contato=true;
			}
		}
	}
}

void setaPosicaoObs(){
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DefineIluminacao();

	// esta funcao controla o ângulo de vizualização
	gluLookAt(150,250,100,0,H*0.7,0,0,1,0);
}

// funcao para fazer toda a simulacao
void massamola(int v)
{
 calcPosicoes();// calcula as posicoes
	glutPostRedisplay();// forca a display executar
	if (modulo(quant_movimento)>100 || !contato){ // para a simulacao
		glutTimerFunc(T,massamola,1); // executa mai uma iteracao
	}
}

// esta funcao serve apenas para colorir a malha
//	proporcionalmente às forcas atuantes em casa particula
void cor(double forca){
	forca/=30.0;
	GLfloat difusa[4]= {0,0,0,0};
	difusa[0] = 0.5*forca;
	difusa[1] = 0.5*(1.0 - forca);
	difusa[2] = 1.0;
	difusa[3] = 1.0;
	glMaterialfv(GL_FRONT,GL_DIFFUSE,difusa);
	difusa[0] = 1.0;
	difusa[1] = 0.5*forca;
	difusa[2] = 0.5*(1.0 - forca);
	difusa[3] = 1.0;
	glMaterialfv(GL_BACK,GL_DIFFUSE,difusa);
}

void DesenhaMalha(void) /// esta funcao desenha a manha, é uma Callback, está no lugar da desenha
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	setaPosicaoObs();
	double deslocamento=N/2;
	glBegin(GL_TRIANGLES);
	for(int i=0;i<N-1;i++){
		for(int j=0;j<N-1;j++){
			cor(modulo(Felastica[i][j]));
			glVertex3f(P[i][j].x-deslocamento,P[i][j].z, P[i][j].y-deslocamento);

			cor(modulo(Felastica[i][j+1]));
			glVertex3f(P[i][j+1].x-deslocamento,P[i][j+1].z, P[i][j+1].y-deslocamento);

			cor(modulo(Felastica[i+1][j]));
			glVertex3f(P[i+1][j].x-deslocamento,P[i+1][j].z, P[i+1][j].y-deslocamento);
			cor(modulo(Felastica[i][j+1]));
			glVertex3f(P[i][j+1].x-deslocamento,P[i][j+1].z, P[i][j+1].y-deslocamento);



			cor(modulo(Felastica[i+1][j+1]));
			glVertex3f(P[i+1][j+1].x-deslocamento,P[i+1][j+1].z, P[i+1][j+1].y-deslocamento);
			cor(modulo(Felastica[i+1][j]));
			glVertex3f(P[i+1][j].x-deslocamento,P[i+1][j].z, P[i+1][j].y-deslocamento);
			//imprime(P[i][j]);
		}
	}
	glEnd();

	GLfloat difusa[4]={0,0,1.0,1.0};
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,difusa);
	glTranslatef(0,H*0.8,0);
	glutSolidSphere(R,30,30);

	// Executa os comandos OpenGL
	glutSwapBuffers();
}

// Função callback chamada para gerenciar eventos de teclas
void Teclado(unsigned char key, int x, int y)
{
	if (key == 27)  // sai comm ESC
		exit(0);
}

// Função responsável por inicializar parâmetros e variáveis
void Inicializa(void)
{
	// Define a cor de fundo da janela de visualização como branca
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	//glClearDepth(1.0f);
	setMesh();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);

	// Define do modo de operação da GLUT
	// GLUT_SINGLE significa que vai usar 1 buffer só (se for usar animação ou 3D use GLUT_DOUBLE)
	// aqui, como o código é 2D usamos 1 buffer
	// GLUT_RGB significa que cada pixel é RGB
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

	// Especifica a posição inicial da janela GLUT
	glutInitWindowPosition(50, 0);

	// Especifica o tamanho inicial em pixels da janela GLUT
	glutInitWindowSize(1200, 1200);

	// Cria a janela passando como argumento o título da mesma
	glutCreateWindow("Simulação Tecidos");

	// Registra a função callback de redesenho da janela de visualização
	glutDisplayFunc(DesenhaMalha);

    glutTimerFunc(T,massamola,1);

	// Registra a função callback de redimensionamento da janela de visualização
	glutReshapeFunc(AlteraTamanhoJanela);

	// Registra a função callback para tratamento das teclas ASCII
	glutKeyboardFunc(Teclado);

	// Chama a função responsável por fazer as inicializações
	Inicializa();

	// Inicia o processamento e aguarda interações do usuário
	glutMainLoop();

	return 0;
}
