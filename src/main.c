#include "device_driver.h"

#define LCDW			(320)
#define LCDH			(240)

// 각 픽셀 크기 정의
#define X_life	 		(41)
#define X_score	 		(279)
#define PLAY_X_MIN   	(X_life)
#define PLAY_X_MAX   	(X_score - POLICE_SIZE_X)
#define PLAY_X_MAX_POLICE (X_score - POLICE_SIZE_X)
#define PLAY_X_MAX_SHOES  (X_score - SHOES_SIZE_X)
#define X_MAX  			(LCDW)
#define Y_MIN	 		(0)
#define Y_MAX	 		(LCDH - 1)

// 각 개체의 주기 & 속도
#define TIMER_PERIOD	(100)
#define SHOES_SPAWN   	(7000)
#define BULLET_PERIOD   (80)
#define BULLET_SPAWN    (3000)

// 각 개체 크기
#define THIEF_STEP		(10)
#define THIEF_SIZE_X	(20)
#define THIEF_SIZE_Y	(20)

#define POLICE_STEP		(10)
#define POLICE_SIZE_X	(20)
#define POLICE_SIZE_Y	(20)

#define SHOES_STEP		(10)
#define SHOES_SIZE_X	(10)
#define SHOES_SIZE_Y	(10)

#define BULLET_STEP       (15)
#define BULLET_SIZE_X     (10)
#define BULLET_SIZE_Y     (5)


//색상 정의
#define BACK_COLOR		(5)
#define THIEF_COLOR		(0)
#define POLICE_COLOR	(3)
#define SHOES_COLOR 	(1)
#define WALL_COLOR  	(4)
#define BULLET_COLOR    (2)

// 생명 개수
#define MAX_LIVES       (3)

//게임 끝
#define GAME_OVER		(1)

// QUERY_DRAW 구조체 및 변수들
typedef struct
{
	int x,y; // 조이 스틱 방향
	int w,h; // 객체 크기
	int ci; // 색상
	
}QUERY_DRAW;

static QUERY_DRAW thief;
static QUERY_DRAW police[2];
static QUERY_DRAW shoes;
static QUERY_DRAW heart;
static QUERY_DRAW bullet;
static int score;
static int lives;
static int i;
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};
static int shoe_spawn_elapsed_ms = 0; // 누적된 시간
static int shoes_active = 0; // 신발이 화면에 있는지 여부
static int thief_speed = THIEF_STEP;
static int game_level = 1;  // 초기 레벨 1
static int bullet_active = 0;
static int bullet_spawn_timer = 0;
static int police_period_mul = 10;    // 이동 인터럽트 주기 multiplier
static int speed_counter = 0;     // 몇 초 지났는지 카운트
static int elapsed_ms = 0;     // 누적된 밀리초

extern volatile int TIM1_Expired;
extern volatile int TIM4_expired;
extern volatile int TIM2_expired;
extern volatile int USART1_rx_ready;
extern volatile int USART1_rx_data;
extern volatile int Jog_key_in;
extern volatile int Jog_key;


// 함수
static int IsOverlap(QUERY_DRAW *a, QUERY_DRAW *b);
static int Check_Collision(void);
static void POLICE_Move(QUERY_DRAW *p);
static void k0(void);
static void k1(void);
static void k2(void);
static void k3(void);
static int THIEF_Move(int k);
static void Draw_Object(QUERY_DRAW * obj);
static void Draw_Lives(void);
static void Draw_Level(void);
static void Game_Init(void);
void Play_Next_BGM_Note(void);
void System_Init(void);



static int IsOverlap(QUERY_DRAW *a, QUERY_DRAW *b) 
{
    return (a->x < b->x + b->w) && (a->x + a->w > b->x) && (a->y < b->y + b->h) && (a->y + a->h > b->y);
}

static int Check_Collision(void)
{
	for (i = 0; i < 2; i++) 
	{
        if (IsOverlap(&thief, &police[i])) 
		{
			Uart_Printf("Hit by Police! Lives: %d\n", lives);
            return GAME_OVER;
        }
    }

	if (shoes_active) 
	{
        if (IsOverlap(&thief, &shoes)) 
		{
            thief_speed += 1;
			Uart_Printf("Thief Speed Up!\n");
            shoes_active = 0;
            shoe_spawn_elapsed_ms = 0;
            Lcd_Draw_Box(shoes.x, shoes.y, shoes.w, shoes.h, color[BACK_COLOR]);
        } 
		else 
		{
            for (i = 0; i < 2; i++) 
			{
                if (IsOverlap(&police[i], &shoes)) 
				{
                    shoes_active = 0;
                    shoe_spawn_elapsed_ms = 0;
                    Lcd_Draw_Box(shoes.x, shoes.y, shoes.w, shoes.h, color[BACK_COLOR]);
                }
            }
        }
    }

	return 0;
}


// 경찰이 도둑을 따라가는 코드
static void POLICE_Move(QUERY_DRAW *p) 
{

    if (p->x < thief.x) p->x += POLICE_STEP;
    else if (p->x > thief.x) p->x -= POLICE_STEP;

    if (p->y < thief.y) p->y += POLICE_STEP;
    else if (p->y > thief.y) p->y -= POLICE_STEP;

    // 경계 클램핑
    if (p->x < X_life) p->x = X_life;
    else if (p->x + p->w > X_score) p->x = X_score - p->w;

    if (p->y < Y_MIN) p->y = Y_MIN;
    else if (p->y + p->h > Y_MAX) p->y = Y_MAX - p->h;
}


// up
static void k0(void)
{
	if(thief.y - thief_speed <= Y_MIN) thief.y = Y_MIN;
	else if(thief.y > Y_MIN) thief.y -= thief_speed;
}
//down
static void k1(void)
{
	if(thief.y + thief_speed + thief.h >= Y_MAX) thief.y = Y_MAX - thief.h;
	else if(thief.y + thief.h <= Y_MAX) thief.y += thief_speed;
}
//left
static void k2(void)
{
	if(thief.x - thief_speed <= X_life) thief.x = X_life;
	else if(thief.x > X_life) thief.x -= thief_speed;
	
}
//right
static void k3(void)
{
	if (thief.x + thief_speed + thief.w > X_score) thief.x = X_score - thief.w;	
    else thief.x += thief_speed;
}

static int THIEF_Move(int k)
{
	// UP(0), DOWN(1), LEFT(2), RIGHT(3)
	static void (*key_func[])(void) = {k0, k1, k2, k3};
	if(k <= 3) key_func[k]();

	return 0;
}


static void Draw_Object(QUERY_DRAW * obj)
{
	Lcd_Draw_Box(obj->x, obj->y, obj->w, obj->h, color[obj->ci]);
}

static void Draw_Lives(void) 
{
    Lcd_Printf(5,15, color[BACK_COLOR],color[WALL_COLOR],1,1,"Life");

	heart.w = SHOES_SIZE_X + 10;
	heart.h = SHOES_SIZE_Y + 10;
	heart.ci = THIEF_COLOR;
    for (i = 0; i < MAX_LIVES; i++) 
	{
		heart.x = 10;
		heart.y = 50 + i*(SHOES_SIZE_X + 50);
        if (i >= MAX_LIVES - lives) Draw_Object(&heart);
        else Lcd_Draw_Box(heart.x, heart.y, heart.w, heart.h, color[WALL_COLOR]);
    }
}

static void Draw_Level(void)
{
    char level_str[10];
    sprintf(level_str, "%d", game_level);
    
    // 벽 색상으로 벽 영역 초기화 후 레벨 표시
    Lcd_Printf(X_score + 2, 15, color[BACK_COLOR], color[WALL_COLOR], 1, 1, "Level");
    Lcd_Printf(X_score + 15, (Y_MAX / 2) - 8, color[BACK_COLOR], color[WALL_COLOR], 2, 2, level_str);
}

static void Game_Init(void)
{
	score = 0;
	lives = MAX_LIVES;
	shoes_active = 0;
    shoe_spawn_elapsed_ms = 0;
    thief_speed = THIEF_STEP;
	elapsed_ms = 0; 
	police_period_mul = 10;
    speed_counter = 0;
	game_level = 1;
	bullet_active = 0;
    bullet_spawn_timer = 0;

	Lcd_Clr_Screen();

	thief.x = 150; thief.y = 110; thief.w = THIEF_SIZE_X; thief.h = THIEF_SIZE_Y; thief.ci = THIEF_COLOR;
	
	for (i = 0; i < 2; i++) 
	{
        police[i].w = POLICE_SIZE_X;
        police[i].h = POLICE_SIZE_Y;
        police[i].ci = POLICE_COLOR;

        do {
            police[i].x = PLAY_X_MIN + rand() % (PLAY_X_MAX_POLICE - PLAY_X_MIN + 1);
            police[i].y = rand() % (Y_MAX - police[i].h + 1);
        } while (IsOverlap(&thief, &police[i]) || (i>0 && IsOverlap(&police[0], &police[1])));

    }

	Lcd_Draw_Box(0, 0, X_life, Y_MAX, color[WALL_COLOR]);
	Lcd_Draw_Box(X_score, 0, X_MAX, Y_MAX, color[WALL_COLOR]);
	Draw_Lives();
	Draw_Level();
	Draw_Object(&thief);
    for (i = 0; i < 2; i++) Draw_Object(&police[i]);
	TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD * police_period_mul);	
	TIM2_Repeat_Interrupt_Enable(1, BULLET_PERIOD);
}


#define BASE  (500) // msec 단위
int bgm_index = 0;

enum key {
    C1, C1_, D1, D1_, E1, F1, F1_, G1,
    G1_, A1, A1_, B1, C2, C2_, D2, D2_,
    E2, F2, F2_, G2, G2_, A2, A2_, B2
};

enum note {
    N16 = BASE/4, N8 = BASE/2, N4 = BASE,
    N2 = BASE*2, N1 = BASE*4
};

static const unsigned short tone_value[] = {
    523, 554, 587, 622, 659, 698, 739, 783,
    830, 880, 932, 987, 1046, 1108, 1174, 1244,
    1318, 1396, 1479, 1567, 1661, 1760, 1864, 1975
};

const int song1[][2] = {
    {G1, N4}, {E1, N8}, {E1, N8}, {G1, N8}, {E1, N8}, {C1, N4},
    {D1, N4}, {E1, N8}, {D1, N8}, {C1, N8}, {E1, N8}, {G1, N4},

    {C2, N8}, {G1, N16}, {C2, N8}, {G1, N8}, {C2, N8}, {G1, N8}, {E1, N4},
    {G1, N4}, {D1, N8}, {F1, N8}, {E1, N8}, {D1, N8}, {C1, N4}
};
const int song_length = sizeof(song1) / sizeof(song1[0]);

void Play_Next_BGM_Note(void)
{
    if (bgm_index >= song_length) bgm_index = 0;

    unsigned char tone = song1[bgm_index][0];
    int duration = song1[bgm_index][1];

	TIM3_Out_Freq_Generation(tone_value[tone]);
    TIM1_Delay_Int(1, duration); 
    bgm_index++;
}


void System_Init(void)
{
	Clock_Init();
	LED_Init();
	Key_Poll_Init();
	Uart1_Init(115200);

	SCB->VTOR = 0x08003000;
	SCB->SHCSR = 7<<16;
}

void Main(void)
{
	Lcd_Init(3);
	Jog_Poll_Init();
	Jog_ISR_Enable(1);
	Uart1_RX_Interrupt_Enable(1);
	
	Lcd_Clr_Screen(); 
	TIM3_Out_Init();             

	volatile int seed_counter = 0;
	int result = 0;
	int play_music = 0;

	for(;;)
	{
		Lcd_Clr_Screen();
		Lcd_Printf(50,50, color[WALL_COLOR],color[BACK_COLOR],2,2,"!!~Welcome~!!");
		Lcd_Printf(80,150, color[WALL_COLOR],color[BACK_COLOR],1,1,"Move Jog to Start!!");

		while (!Jog_key_in)
		{
    		seed_counter++;
			if (TIM1_Expired || play_music == 0)
			{
				play_music = 1;
				Play_Next_BGM_Note();
				TIM1_Expired = 0;
			}
		}	
		srand(seed_counter); 
		Jog_Wait_Key_Released();

		Game_Init(); // 게임 초기화
		Uart_Printf("Game Start!\n");

		for(;;)
		{
			if (TIM1_Expired || play_music == 0)
			{
				play_music = 1;
				Play_Next_BGM_Note();
				TIM1_Expired = 0;
			}
			// 조이 스틱 입력 처리리
			if(Jog_key_in) 
			{	
				
				thief.ci = BACK_COLOR;
				Draw_Object(&thief);

				
				THIEF_Move(Jog_key);

				
				thief.ci = THIEF_COLOR;
				Draw_Object(&thief);

				
				Jog_key_in = 0;
			}

			if (TIM2_expired)
			{
				TIM2_expired = 0;

				if (bullet_active)
				{
					bullet.ci = BACK_COLOR;
					Draw_Object(&bullet);
					bullet.x -= BULLET_STEP;

					if (bullet.x + bullet.w < PLAY_X_MIN)
					{
						bullet_active = 0;
					}
					else
					{
						bullet.ci = BULLET_COLOR;
						Draw_Object(&bullet);
					}

					if (IsOverlap(&bullet, &thief))
					{
						lives--;
						bullet_active = 0;
						Draw_Lives();
						Uart_Printf("Hit by Bullet! Lives: %d\n", lives);

						if (lives <= 0)
						{
							TIM4_Repeat_Interrupt_Enable(0, 0);
							TIM2_Repeat_Interrupt_Enable(0, 0);
							Uart_Printf("Game Over\n");
							Lcd_Clr_Screen();
							Lcd_Printf(50,50, color[WALL_COLOR],color[BACK_COLOR],2,2,"!!Game Over!!");
							Lcd_Printf(80,150, color[WALL_COLOR],color[BACK_COLOR],1,1,"Move Jog to Go Back");
							while (!Jog_key_in)
							{
								if (TIM1_Expired || play_music == 0)
								{
									play_music = 1;
									Play_Next_BGM_Note();
									TIM1_Expired = 0;
								}
							}
							Jog_Wait_Key_Released();
							Jog_key_in = 0;
							break;
						}
					}
				}
				else
				{
					bullet_spawn_timer += BULLET_PERIOD;

					if (bullet_spawn_timer >= BULLET_SPAWN)
					{
						bullet_spawn_timer = 0;
						bullet_active = 1;
						bullet.x = X_score - BULLET_SIZE_X;
						bullet.y = Y_MIN + rand() % (Y_MAX - BULLET_SIZE_Y);
						bullet.w = BULLET_SIZE_X;
						bullet.h = BULLET_SIZE_Y;
						bullet.ci = BULLET_COLOR;
						Draw_Object(&bullet);
					}
				}
			}

			if (TIM4_expired) 
			{
				TIM4_expired = 0;
			
				for(i=0;i<2;i++)
				{ 
					police[i].ci=BACK_COLOR;
					Draw_Object(&police[i]); 
				}
              
                for(i=0;i<2;i++) POLICE_Move(&police[i]);
                
                for(i=0;i<2;i++)
				{ 
					police[i].ci=POLICE_COLOR;
					Draw_Object(&police[i]); 
				}

				if (!shoes_active) 
				{
                    shoe_spawn_elapsed_ms += TIMER_PERIOD * police_period_mul;

                    if (shoe_spawn_elapsed_ms >= SHOES_SPAWN) 
					{
                        shoe_spawn_elapsed_ms -= SHOES_SPAWN;
                        shoes_active = 1;
                        shoes.x = PLAY_X_MIN + rand() % (PLAY_X_MAX_SHOES - PLAY_X_MIN + 1);
                        shoes.y = Y_MIN + rand() % (Y_MAX - SHOES_SIZE_Y + 1);
                        shoes.w = SHOES_SIZE_X; 
						shoes.h = SHOES_SIZE_Y; 
						shoes.ci = SHOES_COLOR;
                        Draw_Object(&shoes);
                    }
                }
				
				//시간 누적: 이번 인터럽트가 발생한 간격만큼 더함, 1초(1000ms) 단위로 초 카운트
				elapsed_ms += TIMER_PERIOD * police_period_mul;  // 예: 100ms * multiplier

				if (elapsed_ms >= 1000) 
				{
					elapsed_ms -= 1000;        // 남은 ms는 그대로 누적
					speed_counter++;           // 1초 경과
			
					// 10초마다 속도(=주기)를 한 단계 당김
					if (speed_counter >= 10) 
					{
						speed_counter = 0;

						if (police_period_mul > 1) 
						{
							police_period_mul--;
							game_level++;  
							Draw_Level();  

							// 타이머 재설정
							TIM4_Repeat_Interrupt_Enable(0, 0);
							TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD * police_period_mul);
							Uart_Printf("Police Speed Up!!\n");
						}
						if (police_period_mul == 1)
						{
							Lcd_Clr_Screen();
							TIM4_Repeat_Interrupt_Enable(0, 0);
							TIM2_Repeat_Interrupt_Enable(0, 0);
							Uart_Printf("Thief Win!\n");
							Lcd_Printf(10,50, color[WALL_COLOR],color[BACK_COLOR],2,2,"!!Congratulations!!");
							Lcd_Printf(80,150, color[WALL_COLOR],color[BACK_COLOR],1,1,"Move Jog to Go Back");
							while (!Jog_key_in)
							{
								if (TIM1_Expired || play_music == 0)
								{
									play_music = 1;
									Play_Next_BGM_Note();
									TIM1_Expired = 0;
								}
							}
							Jog_Wait_Key_Released();
							Jog_key_in = 0;
							break;							
						}
					}
				}

				result = Check_Collision();
			}

			// 게임 오버 처리
			if(result)
			{
                lives--;
                Draw_Lives();

                if(lives<=0)
				{
                    TIM4_Repeat_Interrupt_Enable(0,0);
					TIM2_Repeat_Interrupt_Enable(0,0);
                    Uart_Printf("Game Over\n");
					Lcd_Clr_Screen();
					Lcd_Printf(50,50, color[WALL_COLOR],color[BACK_COLOR],2,2,"!!Game Over!!");
					Lcd_Printf(80,150, color[WALL_COLOR],color[BACK_COLOR],1,1,"Move Jog to Go Back");
					while (!Jog_key_in)
					{
						if (TIM1_Expired || play_music == 0)
						{
							play_music = 1;
							Play_Next_BGM_Note();
							TIM1_Expired = 0;
						}
					}
					Jog_Wait_Key_Released();
					Jog_key_in = 0;
                    break;  
                } 

				else 
				{
					result = 0; 
                }
            }
		}
	}
}
