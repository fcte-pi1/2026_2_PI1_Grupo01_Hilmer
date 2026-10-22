import os
import sys
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, KeepTogether, PageBreak, HRFlowable
)
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super(NumberedCanvas, self).__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super(NumberedCanvas, self).showPage()
        super(NumberedCanvas, self).save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#64748B"))
        
        # Header (pages > 1)
        if self._pageNumber > 1:
            self.drawString(36, 810, "Micromouse PI1 — Roteiro de Testes de Bancada e Validação de Energia")
            self.drawRightString(559, 810, "Rev 2.0 • 3x Sensores ToF")
            self.setStrokeColor(colors.HexColor("#CBD5E1"))
            self.setLineWidth(0.5)
            self.line(36, 804, 559, 804)
        
        # Footer (all pages)
        self.setStrokeColor(colors.HexColor("#CBD5E1"))
        self.setLineWidth(0.5)
        self.line(36, 38, 559, 38)
        self.drawString(36, 26, "UnB FGA • Projeto Integrador 1 • Grupo 01 (Hilmer)")
        page_str = f"Página {self._pageNumber} de {page_count}"
        self.drawRightString(559, 26, page_str)
        self.restoreState()

def build_pdf(filename):
    doc = SimpleDocTemplate(
        filename,
        pagesize=A4,
        leftMargin=36,
        rightMargin=36,
        topMargin=46,
        bottomMargin=46
    )

    styles = getSampleStyleSheet()
    
    # Custom styles
    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=17,
        leading=21,
        textColor=colors.HexColor('#0F172A'),
        spaceAfter=3
    )
    
    subtitle_style = ParagraphStyle(
        'DocSubTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10.5,
        leading=14,
        textColor=colors.HexColor('#2563EB'),
        spaceAfter=8
    )

    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=colors.HexColor('#0F172A'),
        spaceBefore=12,
        spaceAfter=6,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10,
        leading=13,
        textColor=colors.HexColor('#1E3A8A'),
        spaceBefore=8,
        spaceAfter=4,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.5,
        leading=11.5,
        textColor=colors.HexColor('#1E293B')
    )

    body_bold = ParagraphStyle(
        'Body_Bold',
        parent=body_style,
        fontName='Helvetica-Bold'
    )

    notice_style = ParagraphStyle(
        'Notice_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=colors.HexColor('#92400E')
    )

    expected_style = ParagraphStyle(
        'Expected_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=colors.HexColor('#14532D')
    )

    table_header_style = ParagraphStyle(
        'TH_Style',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=8,
        leading=10,
        textColor=colors.HexColor('#0F172A')
    )

    table_cell_style = ParagraphStyle(
        'TD_Style',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.5,
        leading=9.5,
        textColor=colors.HexColor('#1E293B')
    )

    table_cell_bold = ParagraphStyle(
        'TD_Bold',
        parent=table_cell_style,
        fontName='Helvetica-Bold'
    )

    story = []

    # Title Block
    story.append(Paragraph("ROTEIRO DE TESTES — SUBSISTEMA DE ENERGIA & HARDWARE", title_style))
    story.append(Paragraph("Micromouse PI1 • Validação Sequencial e Segura de Bancada • Rev 2.0 Corrigida", subtitle_style))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor('#2563EB'), spaceAfter=8))

    # Warning Box
    warn_text = Paragraph(
        "<b>REGRA DE OURO DE SEGURANÇA:</b> JAMAIS conecte a placa ESP32-C3, os sensores VL53L0X, a IMU MPU6050 ou o driver DRV8833 sem antes medir com multímetro as saídas de cada regulador Step-Down MP1584EN (3,3V, 5,0V e 6,0V). A tensão bruta da bateria (8,4V) provoca destruição eletrotérmica imediata dos circuitos lógicos sensíveis de 3,3V e 5V.",
        notice_style
    )
    warn_table = Table([[warn_text]], colWidths=[523])
    warn_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor('#FEF3C7')),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor('#F59E0B')),
        ('PADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(warn_table)
    story.append(Spacer(1, 8))

    # Section 1: Overview and Hardware Matrix
    story.append(Paragraph("1. Matriz Resumo dos Componentes e Tensões de Operação", h1_style))
    matrix_data = [
        [
            Paragraph("Componente", table_header_style),
            Paragraph("Tensão / Pino", table_header_style),
            Paragraph("Fonte Alimentadora", table_header_style),
            Paragraph("Consumo Típico", table_header_style),
            Paragraph("Função e Cuidados de Teste", table_header_style)
        ],
        [
            Paragraph("<b>Bateria LiPo 2S</b>", table_cell_style),
            Paragraph("7,4V nom.<br/>8,4V max", table_cell_style),
            Paragraph("Química LiPo / Pack 650 mAh", table_cell_style),
            Paragraph("Até ~3A stall<br/>(48A burst)", table_cell_style),
            Paragraph("Fonte primária com conector XT-30. Tensão não pode baixar de 6,6V (3,3V/célula).", table_cell_style)
        ],
        [
            Paragraph("<b>Fusível PPTC MF-R185</b>", table_cell_style),
            Paragraph("Em série linha (+)", table_cell_style),
            Paragraph("Saída positiva da bateria", table_cell_style),
            Paragraph("I_hold = 1,85A<br/>I_trip = 3,7A", table_cell_style),
            Paragraph("Proteção contra sobrecorrente e curto. Desarma termicamente para estado alta impedância.", table_cell_style)
        ],
        [
            Paragraph("<b>Chave Geral SW1</b>", table_cell_style),
            Paragraph("Em série pós-PPTC", table_cell_style),
            Paragraph("Linha positiva protegida", table_cell_style),
            Paragraph("Suporta até 3A", table_cell_style),
            Paragraph("Chave seccionadora geral. No estado OFF, desliga 100% da eletrônica do robô.", table_cell_style)
        ],
        [
            Paragraph("<b>Buck U5 (MP1584EN)</b>", table_cell_style),
            Paragraph("<b>3,30 V ±0,05V</b>", table_cell_bold),
            Paragraph("VBAT (7,4V – 8,4V)", table_cell_style),
            Paragraph("Até 1A útil<br/>(quiesc. ~30mA)", table_cell_style),
            Paragraph("Alimenta barramento limpo de 3,3V: 3x VL53L0X, MPU6050 e encoders.", table_cell_style)
        ],
        [
            Paragraph("<b>Buck U2 (MP1584EN)</b>", table_cell_style),
            Paragraph("<b>5,00 V ±0,05V</b>", table_cell_bold),
            Paragraph("VBAT (7,4V – 8,4V)", table_cell_style),
            Paragraph("Até 1A útil<br/>(placa ~110mA)", table_cell_style),
            Paragraph("Conectado ao pino 5V (VIN) da ESP32-C3 Super Mini para alimentar seu LDO interno.", table_cell_style)
        ],
        [
            Paragraph("<b>Buck U4 (MP1584EN)</b>", table_cell_style),
            Paragraph("<b>6,00 V ±0,10V</b>", table_cell_bold),
            Paragraph("VBAT (7,4V – 8,4V)", table_cell_style),
            Paragraph("Até 3A pico<br/>(motores ~0,3A)", table_cell_style),
            Paragraph("Alimenta pino VM de potência do DRV8833. Mantém velocidade constante sem ruído na lógica.", table_cell_style)
        ],
        [
            Paragraph("<b>ESP32-C3 Super Mini</b>", table_cell_style),
            Paragraph("5,0 V no VIN<br/>(3,3V lógica)", table_cell_style),
            Paragraph("Saída do Buck U2 (5V)", table_cell_style),
            Paragraph("~110 mA médio<br/>picos 250 mA", table_cell_style),
            Paragraph("Microcontrolador central RISC-V. USB D-/D+ nos pinos 18/19 livres. Gera PWM e gerencia I2C.", table_cell_style)
        ],
        [
            Paragraph("<b>3x VL53L0X (Proximidade)</b>", table_cell_style),
            Paragraph("3,3 V nos pinos VIN", table_cell_style),
            Paragraph("Saída do Buck U5 (3,3V)", table_cell_style),
            Paragraph("~20 mA cada<br/>(~60 mA total)", table_cell_style),
            Paragraph("Substitutos do LiDAR antigo. I2C (SCL=GPIO8, SDA=GPIO9); XSHUT em GPIO0, 1 e 2.", table_cell_style)
        ],
        [
            Paragraph("<b>IMU MPU6050 (GY-521)</b>", table_cell_style),
            Paragraph("3,3 V no pino VCC", table_cell_style),
            Paragraph("Saída do Buck U5 (3,3V)", table_cell_style),
            Paragraph("~4 mA", table_cell_style),
            Paragraph("Giroscópio/acelerômetro de 6 eixos. I2C compartilhado. Pino AD0 conectado ao GND (0x68).", table_cell_style)
        ],
        [
            Paragraph("<b>Driver Ponte H DRV8833</b>", table_cell_style),
            Paragraph("VM = 6,0V<br/>nSLEEP = 6V (R4)", table_cell_style),
            Paragraph("Buck U4 (6V)<br/>Lógica PWM ESP32", table_cell_style),
            Paragraph("Quiescente 3 mA<br/>Pico até 3A", table_cell_style),
            Paragraph("Ponte H dupla MOSFET. Entradas IN1..IN4 nos pinos GPIO10, 3, 20, 21. Capacitor 470µF na linha VM.", table_cell_style)
        ],
        [
            Paragraph("<b>2x Motores N20 + Encoders</b>", table_cell_style),
            Paragraph("Motores: 6V<br/>Encoders: 3,3V", table_cell_style),
            Paragraph("Saídas OUT1..OUT4<br/>Encoders: Buck U5", table_cell_style),
            Paragraph("Motores: 150mA<br/>Encoders: 10mA", table_cell_style),
            Paragraph("Tração e odometria. Encoders nos GPIO4/5 (Esq) e GPIO6/7 (Dir). Alimentar encoders em 3,3V!", table_cell_style)
        ],
        [
            Paragraph("<b>Divisor ADC Bateria</b>", table_cell_style),
            Paragraph("VBAT → max 2,35V", table_cell_style),
            Paragraph("Barramento VBAT pós-SW1", table_cell_style),
            Paragraph("< 0,1 mA", table_cell_style),
            Paragraph("R1=100kΩ, R2=39kΩ, C5=100nF. Mede nível da bateria para alerta de carga e corte UVLO.", table_cell_style)
        ]
    ]

    col_widths_matrix = [90, 65, 85, 75, 208]
    matrix_table = Table(matrix_data, colWidths=col_widths_matrix)
    matrix_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#F1F5F9')),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor('#CBD5E1')),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('LEFTPADDING', (0,0), (-1,-1), 4),
        ('RIGHTPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(matrix_table)
    story.append(Spacer(1, 10))

    # Page Break for test execution steps
    story.append(PageBreak())

    # Section 2: Step-by-Step Test Procedures
    story.append(Paragraph("2. Roteiro Passo a Passo de Testes Individuais de Bancada", h1_style))
    story.append(Paragraph("Execute rigorosamente na ordem numérica apresentada abaixo:", body_style))
    story.append(Spacer(1, 6))

    tests = [
        {
            "id": "TESTE 01",
            "title": "Inspeção de Continuidade e Não-Curto (Circuito Desenergizado)",
            "tag": "CRÍTICO - SEGURANÇA",
            "tag_color": "#DC2626",
            "equip": "Multímetro Digital (modo continuidade sonora / Ω). Circuito sem bateria.",
            "steps": [
                "Verificar ausência de curto entre o terminal positivo do conector da bateria e o plano de terra unificado (GND).",
                "Verificar ausência de curto entre as três saídas dos reguladores (trilhos 3,3V, 5V e 6V) e o GND.",
                "Checar chave SW1: continuidade (0,0 Ω) em posição ON e circuito aberto (> 1 MΩ) em posição OFF.",
                "Confirmar unificação do plano de terra: continuidade de 0,0 Ω entre o GND do conector de bateria, os GNDs dos 3 reguladores, o GND da ESP32, dos sensores e do driver DRV8833."
            ],
            "expected": "Resistência aberta (> 100 kΩ) entre positivo e terra em todos os nós. Continuidade plena (0,0 Ω) no GND unificado."
        },
        {
            "id": "TESTE 02",
            "title": "Bateria LiPo 2S, Conector XT-30 e Fusível PPTC",
            "tag": "ENERGIA PRIMÁRIA",
            "tag_color": "#2563EB",
            "equip": "Multímetro Digital (escala 20V DC). Bateria LiPo 2S carregada.",
            "steps": [
                "Medir a tensão total nos terminais do conector XT-30 da bateria.",
                "Medir a tensão individual das células no conector de balanceamento JST-XHR (Pinos 1-2 e 2-3).",
                "Conectar a bateria com a chave SW1 desligada (OFF). Medir a tensão antes e depois do fusível PPTC (MF-R185).",
                "Acionar a chave SW1 (sem carga nos reguladores) e medir a tensão VBAT entregue na entrada dos 3 conversores."
            ],
            "expected": "Tensão total entre 7,4V (nominal) e 8,4V (plena carga). Diferença entre células < 0,05V (50 mV). Queda de tensão no fusível < 5 mV. VBAT após chave idêntico à bateria."
        },
        {
            "id": "TESTE 03",
            "title": "Calibração e Regulação dos 3 Conversores Step-Down MP1584EN (A Vazio)",
            "tag": "CRÍTICO - CALIBRAÇÃO",
            "tag_color": "#DC2626",
            "equip": "Multímetro Digital (escala 20V DC) e chave de precisão de ajuste de trimpot.",
            "steps": [
                "<b>DESCONECTAR</b> ESP32-C3, sensores VL53L0X, MPU6050 e DRV8833. Somente os módulos MP1584EN energizados.",
                "<b>Regulador U5 (Lógica/Sensores):</b> Ajustar o trimpot até a saída atingir exatamente <b>3,30 V ± 0,05 V</b>.",
                "<b>Regulador U2 (ESP32-C3):</b> Ajustar o trimpot até a saída atingir exatamente <b>5,00 V ± 0,05 V</b>.",
                "<b>Regulador U4 (Motores):</b> Ajustar o trimpot até a saída atingir exatamente <b>6,00 V ± 0,10 V</b>.",
                "Desligar SW1, aguardar 5 segundos e religar. Conferir se todas as 3 tensões retornam aos mesmos valores exatos."
            ],
            "expected": "U5 = 3,30 V | U2 = 5,00 V | U4 = 6,00 V estáveis, sem oscilação ou deriva ao desligar e religar a alimentação."
        },
        {
            "id": "TESTE 04",
            "title": "Estabilidade dos Reguladores Sob Carga Resistiva Fictícia",
            "tag": "ESTABILIDADE TÉRMICA",
            "tag_color": "#D97706",
            "equip": "Multímetro Digital, resistores de potência (33Ω 2W para 3,3V; 10Ω 5W para 5V; 12Ω 10W para 6V).",
            "steps": [
                "Aplicar resistor de 33 Ω na linha de 3,3V (corrente de ~100 mA) por 1 minuto. Medir a tensão regulada.",
                "Aplicar resistor de 10 Ω na linha de 5,0V (corrente de ~500 mA) por 1 minuto. Medir a tensão regulada.",
                "Aplicar resistor de 12 Ω na linha de 6,0V (corrente de ~500 mA) por 1 minuto. Medir a tensão regulada.",
                "Verificar aquecimento tátil dos circuitos integrados MP1584EN e indutores."
            ],
            "expected": "Queda de tensão sob carga menor que 3% (3,3V > 3,22V; 5V > 4,88V; 6V > 5,85V). Temperatura dos CIs moderada (< 45°C), sem zumbido audível."
        },
        {
            "id": "TESTE 05",
            "title": "Divisor Resistivo de Monitoramento da Tensão da Bateria (ADC)",
            "tag": "MONITORAMENTO ADC",
            "tag_color": "#2563EB",
            "equip": "Multímetro Digital ou Fonte DC de bancada ajustável.",
            "steps": [
                "Com o barramento em VBAT = 8,40 V, medir a tensão entre o nó central do divisor (R1=100k, R2=39k) e o GND.",
                "Calcular o fator de atenuação experimental: V_ADC = 8,40 × (39 / 139) = 2,355 V (ou 2,084 V com R2=33k).",
                "Variar a fonte para 7,40 V e 6,60 V e checar a proporcionalidade linear da leitura atenuada.",
                "Verificar eficácia do capacitor C5 (100 nF) na filtragem de ruído de alta frequência."
            ],
            "expected": "Tensão no nó ADC não ultrapassa 2,40 V em carga plena. Tensão em 6,60 V = 1,85 V (R2=39k) ou 1,64 V (R2=33k). Perfeita linearidade."
        },
        {
            "id": "TESTE 06",
            "title": "Alimentação e Consumo da Placa ESP32-C3 Super Mini",
            "tag": "PROCESSAMENTO",
            "tag_color": "#2563EB",
            "equip": "Multímetro Digital em escala de corrente (mA), cabo USB-C de dados e PC.",
            "steps": [
                "Conectar a saída de 5,0 V do Buck U2 ao pino <b>5V (VIN)</b> da placa ESP32-C3 e GND ao GND comum.",
                "Ligar SW1 e verificar acendimento do LED de power da plaquinha.",
                "Medir a tensão no pino <b>3V3</b> da ESP32 (saída do seu regulador LDO interno onboard): deve marcar 3,30 V estável.",
                "Medir a corrente consumida em repouso na linha de 5,0 V.",
                "Conectar via USB-C ao computador e gravar um firmware de teste básico (Blink / Serial Monitor)."
            ],
            "expected": "Tensão do pino 3V3 entre 3,25V e 3,35V. Corrente em repouso de 25 mA a 45 mA. Gravação USB concluída com 100% de sucesso."
        },
        {
            "id": "TESTE 07",
            "title": "Barramento I2C, IMU MPU6050 e 3x Sensores ToF VL53L0X",
            "tag": "SENSORIAMENTO TOF",
            "tag_color": "#2563EB",
            "equip": "ESP32-C3 conectada via USB ao PC, IDE/PlatformIO aberta no Monitor Serial a 115200 bps.",
            "steps": [
                "Alimentar os 3 sensores VL53L0X e a IMU MPU6050 exclusivamente pela linha regulada de <b>3,3 V do Buck U5</b>.",
                "Interligar SCL (GPIO8) e SDA (GPIO9) em paralelo a todos os 4 dispositivos.",
                "Ligar os pinos XSHUT: Esquerdo no GPIO0, Frontal no GPIO1, Direito no GPIO2. Conectar AD0 da MPU6050 ao GND.",
                "Carregar o firmware de inicialização sequencial: manter GPIO0..2 em LOW, subir um de cada vez e reatribuir endereços.",
                "Verificar detecção no I2C Scanner: <b>0x68</b> (MPU6050), <b>0x30</b> (ToF Esq), <b>0x31</b> (ToF Frontal) e <b>0x32</b> (ToF Dir).",
                "Aproximar anteparo a 50 mm, 100 mm e 150 mm e validar a resposta de distância em mm dos 3 sensores."
            ],
            "expected": "Todos os 4 módulos respondem no barramento I2C sem travamento. Leituras ToF com erro < ±5 mm de 30 a 200 mm."
        },
        {
            "id": "TESTE 08",
            "title": "Encoders Incrementais de Quadratura dos Motores N20",
            "tag": "ODOMETRIA",
            "tag_color": "#2563EB",
            "equip": "ESP32-C3 e Monitor Serial com firmware de contagem de pulsos por interrupção.",
            "steps": [
                "<b>ATENÇÃO:</b> Ligar o VCC dos encoders na linha de <b>3,3 V</b> (e não em 5V ou 6V) para manter nível seguro no GPIO.",
                "Conectar Encoders Esquerdo (Canais A/B) aos pinos GPIO4 e GPIO5.",
                "Conectar Encoders Direito (Canais A/B) aos pinos GPIO6 e GPIO7.",
                "Girar manualmente as rodas no sentido horário e anti-horário e acompanhar a contagem de pulsos.",
                "Confirmar decodificação em quadratura x4 sem ruídos ou perda de bordas de sinal."
            ],
            "expected": "Contagem estritamente positiva ao avançar e negativa ao recuar. Nível lógico de sinal não ultrapassa 3,3 V."
        },
        {
            "id": "TESTE 09",
            "title": "Driver DRV8833 e Acionamento de Potência dos Motores (Rodas Livres)",
            "tag": "CRÍTICO - POTÊNCIA",
            "tag_color": "#DC2626",
            "equip": "Robô sobre suporte com rodas suspensas, multímetro monitorando as linhas de 5V e 3,3V.",
            "steps": [
                "Conectar pino VM do DRV8833 à linha de <b>6,0 V do Buck U4</b> com o capacitor eletrolítico de 470 µF / 100 µF em paralelo.",
                "Garantir pull-up de 47 kΩ no pino nSLEEP/EEP para mantê-lo habilitado (nível alto).",
                "Conectar os sinais de PWM do ESP32: IN1=GPIO10, IN2=GPIO3 (Motor Esq) e IN3=GPIO20, IN4=GPIO21 (Motor Dir).",
                "Executar rampa suave de PWM (25%, 50%, 75%, 100%) para frente e para trás.",
                "Monitorar a linha de 5,0V da ESP32 e 3,3V dos sensores durante as partidas e reversões de sentido dos motores."
            ],
            "expected": "Motores giram nos dois sentidos com velocidade proporcional ao PWM. Corrente a vazio: 80 mA a 150 mA por motor. Queda de tensão na linha 5V e 3,3V < 50 mV (zero risco de brownout reset no ESP32)."
        },
        {
            "id": "TESTE 10",
            "title": "Ensaio de Corte Preventivo por Subtensão (UVLO Embarcado)",
            "tag": "PROTEÇÃO QUÍMICA",
            "tag_color": "#D97706",
            "equip": "Fonte DC de bancada ajustável conectada no lugar da bateria, sistema completo operando.",
            "steps": [
                "Ajustar a fonte de bancada inicialmente em 8,40 V com o robô em modo de teste com motores ativos.",
                "Reduzir gradualmente a tensão para 7,40 V (operação nominal normal).",
                "Reduzir a tensão até o limiar de segurança de <b>6,60 V (3,30 V/célula)</b>.",
                "Verificar se o firmware interrompe imediatamente os sinais de PWM dos motores (corte UVLO a 0%) e aciona alarme de bateria fraca antes de atingir o corte do BMS em 6,40 V."
            ],
            "expected": "Corte de tração imediato aos 6,60 V. Eletrônica de controle e telemetria continuam funcionando, preservando a bateria contra descarga profunda."
        }
    ]

    for t in tests:
        test_content = []
        
        # Test Header Table
        header_text = Paragraph(f"<b>{t['id']} — {t['title']}</b>", h2_style)
        badge_text = Paragraph(f"<font color='{t['tag_color']}'><b>[{t['tag']}]</b></font>", table_cell_bold)
        header_tbl = Table([[header_text, badge_text]], colWidths=[400, 123])
        header_tbl.setStyle(TableStyle([
            ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
            ('BOTTOMPADDING', (0,0), (-1,-1), 2),
            ('TOPPADDING', (0,0), (-1,-1), 2),
            ('LEFTPADDING', (0,0), (-1,-1), 0),
            ('RIGHTPADDING', (0,0), (-1,-1), 0),
        ]))
        test_content.append(header_tbl)

        # Equip
        test_content.append(Paragraph(f"<b>Instrumental / Preparação:</b> {t['equip']}", body_style))
        test_content.append(Spacer(1, 3))

        # Steps
        steps_p = []
        for i, s in enumerate(t['steps'], 1):
            steps_p.append(Paragraph(f"<b>[ &nbsp; ] {i}.</b> {s}", body_style))
        
        for sp in steps_p:
            test_content.append(sp)
            test_content.append(Spacer(1, 2))

        # Expected
        exp_box = Table([[Paragraph(f"<b>Critério de Aceitação / Resultado Esperado:</b> {t['expected']}", expected_style)]], colWidths=[523])
        exp_box.setStyle(TableStyle([
            ('BACKGROUND', (0,0), (-1,-1), colors.HexColor('#F0FDF4')),
            ('BOX', (0,0), (-1,-1), 0.75, colors.HexColor('#22C55E')),
            ('PADDING', (0,0), (-1,-1), 4),
        ]))
        test_content.append(Spacer(1, 2))
        test_content.append(exp_box)
        test_content.append(Spacer(1, 8))

        story.append(KeepTogether(test_content))

    # Page Break for the Checklist Sign-off Table
    story.append(PageBreak())
    story.append(Paragraph("3. Folha de Registro e Visto de Bancada (Homologação Final)", h1_style))
    story.append(Paragraph("Esta folha deve ser impressa e preenchida manualmente na bancada durante a validação dos subsistemas de energia e hardware:", body_style))
    story.append(Spacer(1, 6))

    check_data = [
        [
            Paragraph("Ensaio de Bancada", table_header_style),
            Paragraph("Parâmetro Crítico", table_header_style),
            Paragraph("Valor Medido", table_header_style),
            Paragraph("Resultado", table_header_style),
            Paragraph("Data / Visto", table_header_style)
        ],
        [
            Paragraph("01. Continuidade e GND", table_cell_style),
            Paragraph("R > 100 kΩ / GND = 0 Ω", table_cell_style),
            Paragraph("R = __________ Ω", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("02. LiPo 2S & Fusível PPTC", table_cell_style),
            Paragraph("7,4V – 8,4V | ΔV < 50mV", table_cell_style),
            Paragraph("C1: _____V | C2: _____V<br/>Total: _____V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("03. Buck U5 (Sensores/Lógica)", table_cell_style),
            Paragraph("3,30 V ± 0,05 V", table_cell_style),
            Paragraph("V_out = ________ V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("04. Buck U2 (ESP32-C3)", table_cell_style),
            Paragraph("5,00 V ± 0,05 V", table_cell_style),
            Paragraph("V_out = ________ V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("05. Buck U4 (Motores DRV8833)", table_cell_style),
            Paragraph("6,00 V ± 0,10 V", table_cell_style),
            Paragraph("V_out = ________ V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("06. Estabilidade sob Carga", table_cell_style),
            Paragraph("Queda < 3% / T < 45°C", table_cell_style),
            Paragraph("3V3: ___V | 5V: ___V<br/>6V: ___V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("07. Divisor Monitoramento ADC", table_cell_style),
            Paragraph("V_adc ≤ 2,40 V a 8,4V", table_cell_style),
            Paragraph("V_adc = ________ V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("08. ESP32-C3 Super Mini", table_cell_style),
            Paragraph("3V3 onboard / USB OK", table_cell_style),
            Paragraph("I_rep = ________ mA", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("09. I2C (MPU6050 + 3x VL53)", table_cell_style),
            Paragraph("0x68, 0x30, 0x31, 0x32", table_cell_style),
            Paragraph("Dispositivos detectados:<br/>[ &nbsp; ] 68 [ &nbsp; ] 30 [ &nbsp; ] 31 [ &nbsp; ] 32", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("10. Encoders N20 (3,3V)", table_cell_style),
            Paragraph("Quad x4 nos GPIO4..7", table_cell_style),
            Paragraph("Pulsos por rotação:<br/>Esq: _____ | Dir: _____", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("11. Driver DRV8833 & Motores", table_cell_style),
            Paragraph("Giro bidirecional / Sag < 50mV", table_cell_style),
            Paragraph("I_vazio = ________ mA<br/>ΔV_sag = ________ mV", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ],
        [
            Paragraph("12. Corte UVLO Firmware", table_cell_style),
            Paragraph("Parada de PWM aos 6,60 V", table_cell_style),
            Paragraph("V_corte_real = ______ V", table_cell_style),
            Paragraph("[ &nbsp; ] Aprovado<br/>[ &nbsp; ] Reprovado", table_cell_style),
            Paragraph("____/____/2026<br/>Ass: _____________", table_cell_style)
        ]
    ]

    check_table = Table(check_data, colWidths=[110, 105, 125, 85, 98])
    check_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#F1F5F9')),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor('#CBD5E1')),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('LEFTPADDING', (0,0), (-1,-1), 4),
        ('RIGHTPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(check_table)
    story.append(Spacer(1, 14))

    concl_box = Table([[Paragraph(
        "<b>HOMOLOGAÇÃO DA EQUIPE:</b> Os testes acima foram realizados de acordo com os procedimentos técnicos de segurança de bancada. O hardware do Micromouse encontra-se validado energeticamente para operação autônoma sem riscos de sobrecorrente, inversão ou queima de sensores.",
        body_style
    )]], colWidths=[523])
    concl_box.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor('#F8FAFC')),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor('#94A3B8')),
        ('PADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(concl_box)

    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"PDF gerado com sucesso: {filename}")

if __name__ == '__main__':
    target = '/home/efaria/UnB/PI1/docs/07-testes/Roteiro_de_Testes_Hardware_Energia_Micromouse.pdf'
    build_pdf(target)
