document.addEventListener('DOMContentLoaded', () => {
    const chatBox = document.getElementById('chat-box');
    const userInput = document.getElementById('user-input');
    const sendBtn = document.getElementById('send-btn');
    const initSerialBtn = document.getElementById('init-serial-btn');

    // 初始化串口
    let port;
    let reader;
    let writer;

    async function initSerial() {
        try {
            port = await navigator.serial.requestPort();
            await port.open({ baudRate: 115200 });

            reader = port.readable.getReader();
            writer = port.writable.getWriter();

            listenToSerial();
        } catch (error) {
            console.error('Error initializing serial port:', error);
        }
    }



async function listenToSerial() {
    let buffer = ''; // 用于存储接收到的数据
    let timeoutId;   // 超时计时器

    const timeoutDuration = 100; // 超时时间（单位：毫秒）
    const textDecoder = new TextDecoder('utf-8'); // 创建 UTF-8 解码器

    while (true) {
        const { value, done } = await reader.read();
        if (done) {
            reader.releaseLock();
            break;
        }

        // 将接收到的字节数据解码为 UTF-8 字符串
        const data = textDecoder.decode(value, { stream: true });
        buffer += data;

        // 重置超时计时器
        if (timeoutId) {
            clearTimeout(timeoutId);
        }

        // 设置新的超时计时器
        timeoutId = setTimeout(() => {
            // 超时后，将 buffer 中的内容作为完整的一段话处理
            if (buffer.trim()) {
                try {
                    // 解析 JSON 数据
                    const jsonData = JSON.parse(buffer);
                    // 提取 content 字段的内容
                    const content = extractContent(jsonData);
                    // 将 content 内容显示
                    appendMessage('bot', content);
                } catch (error) {
                    console.error('JSON 解析失败:', error);
                    appendMessage('bot', '接收到的数据格式不正确');
                }
                buffer = ''; // 清空 buffer
            }
        }, timeoutDuration);
    }
}

/**
 * 提取 JSON 数据中的 content 字段内容
 * @param {Object} jsonData - 输入的 JSON 数据
 * @returns {string} - 返回 content 字段的内容
 */
function extractContent(jsonData) {
    // 检查是否存在 choices 字段，并且 choices 数组不为空
    if (jsonData.choices && jsonData.choices.length > 0) {
        // 返回第一个 choice 的 message.content
        return jsonData.choices[0].message.content;
    }
    // 如果 content 字段不存在，返回空字符串或默认提示
    return '未找到 content 字段';
}
  
    async function sendSerialMessage(message) {
        if (writer) {
            const data = new TextEncoder().encode(message);
            await writer.write(data);
        }
    }

    function appendMessage(sender, message) {
        const messageElement = document.createElement('div');
        messageElement.classList.add('message', `${sender}-message`);

        const contentElement = document.createElement('div');
        contentElement.classList.add('message-content');
        // 使用 marked.js 渲染 Markdown
        contentElement.innerHTML = marked.parse(message);
        // 为代码块添加复制按钮
        addCopyButtons(contentElement);
      
        messageElement.appendChild(contentElement);
        chatBox.appendChild(messageElement);
        chatBox.scrollTop = chatBox.scrollHeight;
    }

     // 为代码块添加复制按钮
    function addCopyButtons(container) {
        const codeBlocks = container.querySelectorAll('pre code');
        codeBlocks.forEach((codeBlock) => {
            const preElement = codeBlock.parentElement;
            const copyButton = document.createElement('button');
            copyButton.classList.add('copy-button');
            copyButton.textContent = 'Copy';
            copyButton.addEventListener('click', () => {
                copyToClipboard(codeBlock.textContent);
                copyButton.textContent = 'Copied!';
                setTimeout(() => {
                    copyButton.textContent = 'Copy';
                }, 2000);
            });
            preElement.style.position = 'relative'; // 设置相对定位
            preElement.appendChild(copyButton);
        });
    }

    // 复制文本到剪贴板
    function copyToClipboard(text) {
        navigator.clipboard.writeText(text).then(() => {
            console.log('Text copied to clipboard');
        }).catch((err) => {
            console.error('Failed to copy text: ', err);
        });
    }
    sendBtn.addEventListener('click', async () => {
        const message = userInput.value.trim();
        if (message) {
            appendMessage('user', message);
            await sendSerialMessage(message);
            userInput.value = '';
        }
    });



    initSerialBtn.addEventListener('click', async () => {
        await initSerial();
    });
// 动态调整 textarea 的高度
    userInput.addEventListener('input', () => {
        userInput.style.height = 'auto'; // 重置高度
        userInput.style.height = `${Math.min(userInput.scrollHeight, 150)}px`; // 设置新高度
    });
     userInput.addEventListener('keydown', async (e) => {
        if (e.key === 'Enter') {
            if (e.shiftKey) {
                // 如果按下 Shift + Enter，插入换行
                return; // 允许默认行为（插入换行）
            } else {
                // 如果只按下 Enter，发送消息
                e.preventDefault(); // 阻止默认换行行为
                const message = userInput.value.trim();
                if (message) {
                    appendMessage('user', message);
                    await sendSerialMessage(message);
                    userInput.value = '';
                    userInput.style.height = 'auto'; // 重置高度
                }
            }
        }
    });
    // 初始化串口连接
    initSerial();
});