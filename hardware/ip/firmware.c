#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>

// Protótipos estáticos (evita avisos de protótipo ausente)
static int64_t sw_dotp(const int32_t a[8], const int32_t b[8]);
static void    hw_write_vectors(const int32_t a[8], const int32_t b[8]);
static void    hw_start(void);
static bool    hw_done(void);
static int64_t hw_result(void);
static int     dotp(void);


static int64_t sw_dotp(const int32_t a[8], const int32_t b[8])
{
    int64_t acc = 0;
    for (int i = 0; i < 8; i++)
        acc += (int64_t)a[i] * (int64_t)b[i];
    return acc;
}

static void hw_write_vectors(const int32_t a[8], const int32_t b[8])
{
    // Acessa os registradores CSR como arrays de 32-bit para simplificar a escrita.
    // O qualificador 'volatile' garante que o compilador não otimizará os acessos,
    // forçando a escrita direta no hardware a cada iteração.
    volatile uint32_t *reg_a = (volatile uint32_t *)CSR_DOTPROD_A0_ADDR;
    volatile uint32_t *reg_b = (volatile uint32_t *)CSR_DOTPROD_B0_ADDR;

    for (int i = 0; i < 8; i++) {
        reg_a[i] = a[i];
        reg_b[i] = b[i];
    }
}

static void hw_start(void)
{
    // Gera um pulso em 'start' para evitar reexecuções involuntárias
    // Caso o bit fique em nível alto até o DONE, o hardware poderia reiniciar
    // automaticamente uma nova operação. Portanto, pulse e depois limpe.
    dotprod_start_write(1);
    // Pequeno atraso para garantir pelo menos 1-2 ciclos de clock do SoC
    for (volatile int i = 0; i < 16; ++i)
    { /* noop */
    }
    dotprod_start_write(0);
}

static bool hw_done(void)
{
    // Nota: o nome gerado pelo LiteX para leitura de um CSRStatus(1, name="done")
    // normalmente é dotp_done_read(). Ajuste aqui caso seu csr.h gere um nome diferente.
    return dotprod_done_read();
}

static int64_t hw_result(void)
{
    uint32_t lo = dotprod_result_lo_read();
    uint32_t hi = dotprod_result_hi_read();
    return ((int64_t)(int32_t)hi << 32) | lo;
}

static int dotp(void)
{
    printf("\nLiteX Dot-Product Accelerator Demo\n");
    printf("===================================\n");

    // Vetores de teste
    int32_t A[8] = {1, -2, 3, -4, 5, -6, 7, -8};
    int32_t B[8] = {8, 7, -6, -5, 4, 3, -2, -1};

    // Software
    int64_t sw = sw_dotp(A, B);
    /* Print 64-bit value as two 32-bit halves to avoid relying on %llX
        which may be unsupported or behave differently in the embedded
        libc. We preserve the same hex width for readability. */
    printf("Software: 0x%08X%08X\n", (unsigned int)((uint64_t)sw >> 32), (unsigned int)((uint64_t)sw & 0xFFFFFFFFU));

    // Hardware
    hw_write_vectors(A, B);
    hw_start();
    while (!hw_done())
        ;
    int64_t hw = hw_result();
    printf("Hardware: 0x%08X%08X\n", (unsigned int)((uint64_t)hw >> 32), (unsigned int)((uint64_t)hw & 0xFFFFFFFFU));

    if (hw == sw)
        printf("[OK] Resultado coincide!\n");
    else
        printf("[ERRO] Resultado diferente!\n");

    // Loop simples para observar via UART
    while (1)
    { /* idle */
    }
    return 0;
}

static char *readstr(void)
{
    char c[2];
    static char s[64];
    static int ptr = 0;

    if (readchar_nonblock())
    {
        c[0] = readchar();
        c[1] = 0;
        switch (c[0])
        {
        case 0x7f:
        case 0x08:
            if (ptr > 0)
            {
                ptr--;
                putsnonl("\x08 \x08");
            }
            break;
        case 0x07:
            break;
        case '\r':
        case '\n':
            s[ptr] = 0x00;
            putsnonl("\n");
            ptr = 0;
            return s;
        default:
            if (ptr >= (sizeof(s) - 1))
                break;
            putsnonl(c);
            s[ptr] = c[0];
            ptr++;
            break;
        }
    }
    return NULL;
}

static char *get_token(char **str)
{
    char *c, *d;

    c = (char *)strchr(*str, ' ');
    if (c == NULL)
    {
        d = *str;
        *str = *str + strlen(*str);
        return d;
    }
    *c = 0;
    d = *str;
    *str = c + 1;
    return d;
}

static void prompt(void)
{
    printf("RUNTIME>");
}

static void help(void)
{
    puts("Available commands:");
    puts("help                            - this command");
    puts("reboot                          - reboot CPU");
    puts("led                             - led test");
    puts("dotp                            - dot-product hardware test");
}

static void reboot(void)
{
    ctrl_reset_write(1);
}

static void toggle_led(void)
{
    int i;
    printf("invertendo led...\n");
    i = leds_out_read();
    leds_out_write(!i);
}

static void console_service(void)
{
    char *str;
    char *token;

    str = readstr();
    if (str == NULL)
        return;
    token = get_token(&str);
    if (strcmp(token, "help") == 0)
        help();
    else if (strcmp(token, "reboot") == 0)
        reboot();
    else if (strcmp(token, "led") == 0)
        toggle_led();
    else if (strcmp(token, "dotp") == 0)
        dotp();
    else if (strcmp(token, "") == 0)
        ;
    else
        printf("unknown command: %s\n", token);

    prompt();
}

int main(void)
{
#ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif
    uart_init();

    printf("Hellorld!\n");
    help();
    prompt();

    while (1)
    {
        console_service();
    }

    return 0;
}