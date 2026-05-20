# Imagem com text-wrap dinâmico no QTextEdit — RESOLVIDO

**Status:** Resolvido em 2026-05-19 pelo Opus. Text-wrap dinâmico funcionando, troca de alinhamento sem creep vertical, overlay posicionado corretamente, gap simétrico entre texto e imagem.

## A solução: QTextFrame nativo

A causa raiz do creep era `cursor.insertHtml("<img align='left'>")`. Mesmo "inline", o Qt envolvia a imagem num bloco/frame implícito a cada chamada, e o `remove + insertHtml` deixava resíduos de layout que somavam píxeis a cada toggle.

Trocamos por `QTextFrame` nativo + `QTextFrameFormat::FloatLeft`/`FloatRight`. Imagem float vive dentro de um frame com `Position` setada explicitamente. Trocar lado vira uma operação atômica: `currentFrame->setFrameFormat(ff)` com nova `Position`. Não toca no fluxo do texto, não recria nada, não acumula nada.

## Estrutura final

### Helper em [src/MainWindow.cpp](src/MainWindow.cpp) (topo, anonymous namespace)

```cpp
void applyFloatFrameStyle(QTextFrameFormat &ff, bool isLeft, int w)
{
    ff.setPosition(isLeft ? QTextFrameFormat::FloatLeft : QTextFrameFormat::FloatRight);
    ff.setBorder(0);
    ff.setPadding(0);
    ff.setLeftMargin(isLeft ? 0 : 16);
    ff.setRightMargin(isLeft ? 16 : 0);
    ff.setTopMargin(4);
    ff.setBottomMargin(4);
    ff.setWidth(QTextLength(QTextLength::FixedLength, w));
}
```

Margem 0 no lado da borda da página (não há nada pra afastar), 16 no lado do texto. Top/bottom = 4 pra dar um respiro vertical.

### `onAddImageRequested`

- **Center:** continua bloco solitário com `AlignHCenter` + `insertImage` (sem frame).
- **Left/Right:** `cursor.insertFrame(ff)` com `applyFloatFrameStyle`, depois `cursor.insertImage(imgFmt)` dentro do frame.

### `changeSelectedImageAlignment`

Dois caminhos:

1. **Fast path (Left ↔ Right):** Detecta `inFloatFrame`, pega `currentFrame->frameFormat()`, aplica `applyFloatFrameStyle` com o novo lado, `currentFrame->setFrameFormat(ff)`. **Atômico — não mexe no documento.** Depois `markContentsDirty(frameStart, docEnd - frameStart)` força o reflow do texto ao redor (Qt não reflowa sozinho após mudar frame format).

2. **Transição (Center ↔ Float):** Aí sim destrói e reconstrói. Remove o frame inteiro (`firstPosition - 1` até `lastPosition + 1`) ou limpa o bloco do center. Reinsere via `insertFrame`+`insertImage` ou `insertImage` em bloco centralizado. Localiza a nova imagem por `c.position() - 1` (logo após o `insertImage`) e atualiza `selectedImageCursor`.

### `changeSelectedImageWidth`

Atualiza `QTextImageFormat::setWidth` na imagem **E** `QTextFrameFormat::setWidth` no frame parent (se houver), pra manter o frame sincronizado. Mesmo `markContentsDirty` pra forçar reflow.

### `detectAlignmentForImage`

Lê o `currentFrame()` do cursor da imagem:
- Frame existe e é float → `FloatLeft`/`FloatRight` direto.
- Sem frame float + block alignment `HCenter` → Center.
- Senão → Left default.

Removi a propriedade custom `QTextFormat::ImageName + 1` da estratégia antiga, não é mais necessária.

### `findImageAt`

Reescrito pra varrer **todos** os blocos do document, não só o bloco onde o caret clicou. Para cada fragmento de imagem, pega o frame parent e usa `documentLayout()->frameBoundingRect(parentFrame)` (ou `blockBoundingRect` se não tem frame float). Aí testa `contains(viewportPos)` no rect ajustado por scroll.

Antes usava `cursorRect` na posição da imagem, que retornava onde o caret "virtualmente" estaria — não onde a imagem é realmente renderizada visualmente após o float. Isso era a causa do bug 2 (overlay aparecia acima da imagem).

### `showOverlayForImage`

Mesma lógica do `findImageAt`: `frameBoundingRect` se a imagem está em frame float, senão `blockBoundingRect`. Centraliza o overlay sobre o rect visual real (`visRect.center().x()`). Simplificado — não tem mais switch por alinhamento.

## Pontos sutis

- **`insertFrame` retorna `QTextFrame*`** e posiciona o cursor dentro do frame, no início. Pra sair: `cursor.setPosition(frame->lastPosition() + 1)`. Não foi necessário no `onAddImageRequested` porque o `endEditBlock` resolve, mas no transição-reconstrói do changeAlignment eu deixo o cursor no frame, pega a posição da imagem inserida via `c.position() - 1` e move o `selectedImageCursor` pra lá.

- **`markContentsDirty` é essencial** após `setFrameFormat`. Sem ele, o texto continua renderizado na disposição antiga até o usuário clicar em algum lugar.

- **`removeSelectedText` pra um frame float** precisa pegar de `firstPosition - 1` até `lastPosition + 1` — as marcas de fronteira do frame estão fora dos `first`/`last` positions internos.

## Arquivos alterados

Só [src/MainWindow.cpp](src/MainWindow.cpp). [src/MainWindow.h](src/MainWindow.h) não precisou de mudança — a assinatura dos métodos é a mesma. ImageOverlay, ImageInsertDialog, main.cpp: intocados.

## Como rodar

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;" + $env:PATH
cmake --build 'c:\mira-writing\mira-cpp\build'

# Pra rodar:
$env:PATH = "C:\Qt\6.8.3\mingw_64\bin;" + $env:PATH
& 'c:\mira-writing\mira-cpp\build\mira-writing.exe'
```

Nota: o Qt está em `6.8.3`, não `6.7.0`. O briefing antigo dizia `6.7.0` mas não é mais o caso.

Boa, Mira.
