import os
import sys
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader
import torchvision.transforms as transforms
from torchvision.datasets import ImageFolder

# =====================================================================
# 1. 定义轻量级数独数字 CNN 模型
# =====================================================================
class SudokuDigitCNN(nn.Module):
    def __init__(self):
        super(SudokuDigitCNN, self).__init__()
        self.features = nn.Sequential(
            nn.Conv2d(3, 16, kernel_size=3, padding=1),
            nn.BatchNorm2d(16),
            nn.ReLU(),
            nn.MaxPool2d(2, 2),  # 224x224 -> 112x112
            
            nn.Conv2d(16, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.MaxPool2d(2, 2),  # 112x112 -> 56x56
            
            nn.Conv2d(32, 64, kernel_size=3, padding=1),
            nn.BatchNorm2d(64),
            nn.ReLU(),
            nn.MaxPool2d(2, 2),  # 56x56 -> 28x28
            
            nn.AdaptiveAvgPool2d((4, 4))  # 强制输出固定大小 64x4x4 = 1024
        )
        self.classifier = nn.Sequential(
            nn.Linear(1024, 128),
            nn.ReLU(),
            nn.Dropout(0.2), 
            nn.Linear(128, 10) 
        )
        
    def forward(self, x):
        x = self.features(x)
        x = torch.flatten(x, 1)
        x = self.classifier(x)
        return x

def shift_label(y):
    return y + 1

def main():
    # =====================================================================
    # 2. 环境与路径配置
    # =====================================================================
    # 完美适配你的 Intel GPU (XPU) 环境进行加速
    if hasattr(torch, 'xpu') and torch.xpu.is_available():
        device = torch.device('xpu')
        print("【设备提示】: 检测到 Intel GPU，已成功启用 XPU 加速训练！")
    else:
        device = torch.device('cpu')
        print("【设备提示】: 未检测到硬件加速，使用 CPU 运行。")

    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    DATA_DIR = os.path.join(os.path.dirname(BASE_DIR), 'Character_Sample')
    
    if not os.path.exists(DATA_DIR):
        print(f"【错误】找不到样本目录: {DATA_DIR}，请检查文件夹名称。")
        sys.exit(1)

    # =====================================================================
    # 3. 数据读入与增强
    # =====================================================================
    train_transform = transforms.Compose([
        transforms.Resize((64, 64)),
        transforms.RandomRotation(8),                 
        transforms.RandomAffine(degrees=0, translate=(0.05, 0.05)), 
        transforms.ToTensor(),
    ])

    try:
        dataset = ImageFolder(root=DATA_DIR, transform=train_transform, target_transform=shift_label)
        train_loader = DataLoader(dataset, batch_size=64, shuffle=True, num_workers=2, pin_memory=True)
        print(f"【数据集】: 成功加载分类样本，共有 {len(dataset)} 张数字图片。")
    except Exception as e:
        print(f"【错误】数据集加载异常: {e}")
        sys.exit(1)

    # =====================================================================
    # 4. 开始训练循环（已调整：增加轮次并引入学习率衰减）
    # =====================================================================
    model = SudokuDigitCNN().to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    
    # 使用余弦退火学习率
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=20)

    epochs = 20  # 优化为20轮，配合余弦退火快速收敛
    print(f"\n开始训练（共计 {epochs} 轮）...")
    model.train()
    
    for epoch in range(epochs):
        running_loss = 0.0
        correct = 0
        total = 0
        
        for inputs, labels in train_loader:
            inputs, labels = inputs.to(device), labels.to(device)
            
            optimizer.zero_grad()
            outputs = model(inputs)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()
            
            running_loss += loss.item() * inputs.size(0)
            _, predicted = torch.max(outputs, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()
            
        # 更新学习率
        scheduler.step()
            
        epoch_loss = running_loss / total
        epoch_acc = (correct / total) * 100
        current_lr = optimizer.param_groups[0]['lr']
        print(f"Epoch [{epoch+1:02d}/{epochs}] - 损失: {epoch_loss:.4f} - 准确率: {epoch_acc:.2f}% - 当前学习率: {current_lr:.5f}")

    # =====================================================================
    # 5. 保存并同步导出 ONNX 模型
    # =====================================================================
    print("\n" + "="*30)
    # 1. 保存 PyTorch 权重
    pth_save_path = os.path.join(BASE_DIR, 'custom_model.pth')
    torch.save(model.state_dict(), pth_save_path)
    print(f"【保存成功】PyTorch 权重参数已存至: {pth_save_path}")

    # 2. 导出为 ONNX 文件
    model.eval()
    onnx_save_path = os.path.join(BASE_DIR, 'custom_model.onnx')
    
    # 将模型与导出的 dummy_input 临时转到 CPU 进行稳定导出
    model.to('cpu')
    dummy_input = torch.randn(1, 3, 64, 64, device='cpu')
    
    torch.onnx.export(
        model, 
        dummy_input, 
        onnx_save_path, 
        export_params=True, 
        opset_version=11,  
        do_constant_folding=True,
        input_names=['input'], 
        output_names=['output'],
        dynamo=False,
    )
    print(f"【导出成功】ONNX 静态模型已存至: {onnx_save_path}")
    print("="*30)

if __name__ == '__main__':
    main()