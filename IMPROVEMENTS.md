# MajestLyrics — Lista de Melhorias

Gerada a partir da análise técnica de 2026-05-27.
Atualizar o status de cada item conforme progresso.

## Legenda

| Campo | Escala | Critério |
|---|---|---|
| **Peso** | 1–5 | Impacto na estabilidade, segurança ou experiência do usuário |
| **Dificuldade** | 1–5 | Esforço de implementação (1 = horas, 5 = semanas) |
| **Tier** | P0–P4 | Prioridade geral de execução |
| **Status** | `[ ]` / `[x]` | Pendente / Concluído |

Ordenação dentro de cada tier: maior peso primeiro; em empate, menor dificuldade primeiro.

---

## P0 — Bugs em Produção (executar imediatamente)

> Defeitos que afetam usuários hoje, com correção de baixo custo.

| # | Melhoria | Peso | Dific. | Arquivo(s) | Status |
|---|---|---|---|---|---|
| 01 | **Colisão de cache por título duplicado** — chave atual é só `ToLower(title)`; dois artistas com mesma faixa colparem no mesmo slot in-memory | 5 | 1 | `include/core/LyricHandler.h`, `src/Plugin.cpp` | `[x]` |
| 02 | **Sem limite no parser XML do ChartLyricsDecoder** — `ExtractXmlTag` faz `substr` em corpo arbitrário sem cap; JSON tem 64 KB, XML não tem nada | 4 | 1 | `include/decoders/ChartLyricsDecoder.h` | `[x]` |
| 03 | **CRLF não normalizado** — `std::getline` remove `\n` mas deixa `\r` no fim de cada linha; conteúdo de APIs com CRLF exibe artefatos na UI | 3 | 1 | `include/decoders/LrcLibDecoder.h`, `ChartLyricsDecoder.h` | `[x]` |

---

## P1 — Infraestrutura (base para tudo que vem depois)

> Sem esses itens, refatorações e novas features operam sem rede de segurança.

| # | Melhoria | Peso | Dific. | Arquivo(s) | Status |
|---|---|---|---|---|---|
| 04 | **Testes automatizados — setup base** — configurar Google Test no `.sln`, projeto `MajestLyricsTests` separado, mocks para `HttpClient` | 5 | 3 | Novo projeto `tests/` | `[ ]` |
| 05 | **Testes de parser e UTF** — casos: UTF-8 inválido, BOM, CRLF/LF, linha vazia, linha > 8 KB, resposta vazia, `"failed"`, `"failed_tls"` | 5 | 2 | `tests/test_parser.cpp`, `tests/test_utf.cpp` | `[ ]` |
| 06 | **Testes de rede** — mock de `HttpClient::Get` para simular respostas de API sem hit real de rede | 4 | 2 | `tests/test_network.cpp` | `[ ]` |
| 07 | **CI/CD — GitHub Actions (Win32)** — build automatizado em push/PR, artefato `gen_majestlyrics.dll` + ZIP de release | 4 | 2 | `.github/workflows/build.yml` | `[ ]` |
| 08 | **UTF-8 inválido com falha explícita** — adicionar flag `MB_ERR_INVALID_CHARS` em `MultiByteToWideChar`; retornar erro distinguível em vez de string silenciosamente vazia | 3 | 2 | `include/util/StringUtil.h`, `include/decoders/LyricDecoder.h` | `[ ]` |
| 09 | **BOM UTF-8 explícito** — detectar e remover sequência `\xEF\xBB\xBF` antes de converter, evitar exibição do caractere BOM no início da letra | 3 | 1 | `include/util/StringUtil.h` | `[ ]` |

---

## P2 — Features Core (produto funcional de qualidade)

> Funcionalidades que definem o que o plugin faz de diferente.

| # | Melhoria | Peso | Dific. | Arquivo(s) | Status |
|---|---|---|---|---|---|
| 10 | **Parser LRC básico** — timestamps `[mm:ss.xx]`, múltiplos timestamps por linha, tags de metadados `[ar:]` `[ti:]`, output: vetor de `{ms, text}` | 5 | 4 | `include/parser/LrcParser.h`, `src/parser/LrcParser.cpp` | `[ ]` |
| 11 | **Cache em disco simples** — gravar `artist_title.lrc` na pasta do plugin; carregar antes de consultar API; invalidar manualmente via botão Refresh | 4 | 3 | `include/cache/DiskCache.h`, `src/cache/DiskCache.cpp` | `[ ]` |
| 12 | **Ativar `syncedLyrics` do LrcLib** — query já retorna o campo; substituir `plainLyrics` por `syncedLyrics` quando disponível; armazenar texto LRC bruto | 4 | 2 | `include/decoders/LrcLibDecoder.h` | `[ ]` |
| 13 | **Build x64** — adicionar configuração `Release|x64` no `.vcxproj`; validar compatibilidade dos valores IPC passados via `WPARAM`/`LPARAM` em 64-bit | 3 | 2 | `MajestLyrics.vcxproj` | `[ ]` |
| 14 | **Validação de compatibilidade WACUP** — os headers WASABI (`api_service`, `api_application`) em `WinampApi.h` não estão presentes no SDK atual; confirmar se são necessários e incluir ou remover | 3 | 2 | `include/winamp/WinampApi.h`, `sdk/` | `[ ]` |
| 15 | **API key do ChartLyrics em settings** — a chave RapidAPI está hardcoded em `ChartLyricsDecoder.h`; mover para `options.txt` e não versionar | 3 | 1 | `include/decoders/ChartLyricsDecoder.h`, `src/Plugin.cpp` | `[ ]` |

---

## P2.5 — Refatoração Técnica (dívida acumulada)

> Não adiciona features, mas reduz o custo das próximas melhorias.

| # | Melhoria | Peso | Dific. | Arquivo(s) | Status |
|---|---|---|---|---|---|
| 16 | **Extrair gerenciamento de settings de `Plugin.cpp`** — isolar `ReadSettingsFile`/`SaveSettings` + variáveis de estado em classe `Settings`; eliminar globais raw | 3 | 3 | `src/Plugin.cpp` → novo `include/core/Settings.h` | `[ ]` |
| 17 | **Desacoplar `EmbedWindow.h` de globais de `Plugin.cpp`** — as externs `ini_file` e `plugin` criam acoplamento direto; passar por parâmetro ou contexto | 3 | 3 | `include/winamp/EmbedWindow.h`, `src/EmbedWindow.cpp` | `[ ]` |
| 18 | **Encapsular estado global de `Plugin.cpp`** — `handler`, `activeSong`, `activeSongLyrics`, `iCurrentLineScrolled`, `lyric_mutex` etc. devem ser membros de uma classe de contexto do plugin | 3 | 3 | `src/Plugin.cpp` | `[ ]` |

---

## P3 — Melhorias de UX (experiência do usuário)

> Dependem de P2 estar concluído, especialmente o parser LRC.

| # | Melhoria | Peso | Dific. | Arquivo(s) | Status |
|---|---|---|---|---|---|
| 19 | **Auto-scroll sincronizado com playback** — usar `IPC_GETOUTPUTTIME` + `WM_TIMER` para avançar a linha em sincronia com o timestamp LRC | 4 | 3 | `src/Plugin.cpp` | `[ ]` |
| 20 | **Highlight da linha atual (efeito karaokê)** — requer parser LRC (#10) e auto-scroll (#19); desenhar linha ativa com cor diferente via `WM_DRAWITEM` ou owner-draw | 4 | 4 | `src/Plugin.cpp`, `res/Resources.rc` | `[ ]` |
| 21 | **Enhanced LRC (`<timestamp>` inline)** — parser para timestamps dentro da linha; necessário para karaokê palavra-a-palavra | 3 | 4 | `include/parser/LrcParser.h` | `[ ]` |
| 22 | **TTL do cache em disco (30 dias)** — verificar data de modificação do arquivo `.lrc` ao carregar; revalidar se expirado | 3 | 2 | `include/cache/DiskCache.h` | `[ ]` |
| 23 | **Múltiplos providers como cadeia configurável** — expor no settings quais providers estão ativos e em que ordem (cache → LrcLib → ChartLyrics → arquivo local) | 3 | 3 | `include/decoders/LyricDecoder.h`, `src/Plugin.cpp` | `[ ]` |

---

## P4 — Organização e Distribuição (nice-to-have)

> Impacto em apresentação e operação open source, não em funcionalidade.

| # | Melhoria | Peso | Dific. | Arquivo(s) | Status |
|---|---|---|---|---|---|
| 24 | **CI/CD — build x64 na matrix** — depende do item #13; adicionar `x64` na matrix do workflow após configurar no `.vcxproj` | 2 | 1 | `.github/workflows/build.yml` | `[ ]` |
| 25 | **GitHub Releases automatizadas** — publicar ZIP com DLL + README em cada tag via `softprops/action-gh-release` | 2 | 1 | `.github/workflows/release.yml` | `[ ]` |
| 26 | **README visual** — adicionar badge de CI, screenshot do plugin em uso, GIF de scroll | 2 | 1 | `README.md` | `[ ]` |
| 27 | **CONTRIBUTING.md** — guia de build local, como rodar testes, convenções de commit | 2 | 1 | `CONTRIBUTING.md` | `[ ]` |
| 28 | **SECURITY.md** — canal de reporte de vulnerabilidades, política de disclosure | 2 | 1 | `SECURITY.md` | `[ ]` |
| 29 | **ISSUE_TEMPLATE** — templates para bug report e feature request | 1 | 1 | `.github/ISSUE_TEMPLATE/` | `[ ]` |
| 30 | **SHA256 como chave do cache em disco** — substituir `artist_title.lrc` por hash; evita colisões em títulos com caracteres especiais | 2 | 2 | `include/cache/DiskCache.h` | `[ ]` |
| 31 | **Mini-mode e overlay** — janela compacta sobreposta ao player; requer redesign de UI significativo | 2 | 4 | `src/EmbedWindow.cpp`, `res/` | `[ ]` |
| 32 | **Installer MSI** — WiX Toolset para empacotamento profissional; ZIP é suficiente para o ecossistema Winamp | 1 | 4 | `installer/` | `[ ]` |

---

## Matriz de Priorização

```
Dificuldade →   1          2          3          4          5
               (horas)   (dias)    (semanas)  (meses)
Peso 5   ↑   #03 #09   #05 #06    #04        #10
             #02 #15   #08

Peso 4       #07        #12 #13    #06 #11    #20
                        #22        #23 #19

Peso 3       #01 #16    #08 #14    #16 #17    #21
                                   #18

Peso 2       #26 #27    #24 #30    #23        #31
             #28 #29    #25

Peso 1                                        #32
```

**Regra prática:** itens no canto superior esquerdo (alto peso, baixa dificuldade) devem ser executados primeiro. Itens no canto inferior direito só após os demais estarem estáveis.

---

## Dependências entre Itens

```
#04 (setup testes) ──────────────► #05, #06
#07 (CI/CD Win32) ───────────────► #24 (x64 matrix), #25 (releases)
#10 (parser LRC) ────────────────► #19 (auto-scroll), #20 (karaokê), #21 (Enhanced LRC)
#11 (cache disco) ───────────────► #22 (TTL), #30 (SHA256)
#12 (syncedLyrics) ──────────────► #10 (dados para o parser)
#13 (build x64) ─────────────────► #24 (matrix CI)
#19 (auto-scroll) ───────────────► #20 (karaokê)
#16 #17 #18 (refatoração) ───────► facilita #23 (providers configuráveis)
```

---

## Resumo por Tier

| Tier | Itens | Total de itens |
|---|---|---|
| P0 — Bugs em produção | #01 #02 #03 | 3 |
| P1 — Infraestrutura | #04 #05 #06 #07 #08 #09 | 6 |
| P2 — Features core | #10 #11 #12 #13 #14 #15 | 6 |
| P2.5 — Refatoração | #16 #17 #18 | 3 |
| P3 — UX | #19 #20 #21 #22 #23 | 5 |
| P4 — Organização | #24 #25 #26 #27 #28 #29 #30 #31 #32 | 9 |
| **Total** | | **32** |
