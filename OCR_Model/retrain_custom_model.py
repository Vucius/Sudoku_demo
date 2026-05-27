import argparse
from pathlib import Path
import os
import shutil

from PIL import Image
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import ConcatDataset, DataLoader, Dataset
import torchvision.transforms as transforms


class SudokuDigitCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(3, 16, kernel_size=3, padding=1),
            nn.BatchNorm2d(16),
            nn.ReLU(),
            nn.MaxPool2d(2, 2),
            nn.Conv2d(16, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.MaxPool2d(2, 2),
            nn.Conv2d(32, 64, kernel_size=3, padding=1),
            nn.BatchNorm2d(64),
            nn.ReLU(),
            nn.MaxPool2d(2, 2),
            nn.AdaptiveAvgPool2d((4, 4)),
        )
        self.classifier = nn.Sequential(
            nn.Linear(1024, 128),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(128, 10),
        )

    def forward(self, x):
        x = self.features(x)
        x = torch.flatten(x, 1)
        return self.classifier(x)


class DigitFolderDataset(Dataset):
    def __init__(self, root: Path, transform, allow_zero: bool):
        self.samples = []
        self.transform = transform
        labels = range(0 if allow_zero else 1, 10)
        for label in labels:
            label_dir = root / str(label)
            if not label_dir.exists():
                continue
            for path in label_dir.iterdir():
                if path.suffix.lower() in {".png", ".jpg", ".jpeg", ".bmp"}:
                    self.samples.append((path, label))

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, index):
        path, label = self.samples[index]
        image = Image.open(path).convert("RGB")
        return self.transform(image), label


def export_onnx(model: nn.Module, output_path: Path):
    model.eval()
    model.cpu()
    dummy_input = torch.randn(1, 3, 224, 224)
    torch.onnx.export(
        model,
        dummy_input,
        str(output_path),
        export_params=True,
        opset_version=11,
        do_constant_folding=True,
        input_names=["input"],
        output_names=["output"],
        dynamo=False,
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-data", required=True)
    parser.add_argument("--retrain-data", required=True)
    parser.add_argument("--base-pth", required=True)
    parser.add_argument("--output-pth", required=True)
    parser.add_argument("--output-onnx", required=True)
    parser.add_argument("--epochs", type=int, default=12)
    parser.add_argument("--batch-size", type=int, default=8)
    parser.add_argument("--learning-rate", type=float, default=1e-4)
    args = parser.parse_args()

    transform = transforms.Compose([
        transforms.Resize((224, 224)),
        transforms.RandomRotation(5),
        transforms.RandomAffine(degrees=0, translate=(0.03, 0.03)),
        transforms.ToTensor(),
    ])

    datasets = []
    base_data = Path(args.base_data)
    retrain_data = Path(args.retrain_data)
    if base_data.exists():
        datasets.append(DigitFolderDataset(base_data, transform, allow_zero=False))
    if retrain_data.exists():
        datasets.append(DigitFolderDataset(retrain_data, transform, allow_zero=True))

    datasets = [dataset for dataset in datasets if len(dataset) > 0]
    if not datasets:
        raise RuntimeError("No training images found.")

    dataset = ConcatDataset(datasets)
    loader = DataLoader(dataset, batch_size=args.batch_size, shuffle=True)

    if hasattr(torch, "xpu") and torch.xpu.is_available():
        device = torch.device("xpu")
        print("【设备提示】: 检测到 Intel GPU，已成功启用 XPU 加速微调！")
    elif torch.cuda.is_available():
        device = torch.device("cuda")
    else:
        device = torch.device("cpu")
        print("【设备提示】: 未检测到硬件加速，使用 CPU 运行。")
    model = SudokuDigitCNN()
    base_pth = Path(args.base_pth)
    if not base_pth.exists() or base_pth.stat().st_size <= 1024:
        raise RuntimeError(f"Valid base weights are required for retraining: {base_pth}")

    state = torch.load(base_pth, map_location="cpu")
    model.load_state_dict(state)
    print(f"Loaded base weights: {base_pth}", flush=True)

    model.to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=args.learning_rate)

    for epoch in range(args.epochs):
        model.train()
        total = 0
        correct = 0
        loss_sum = 0.0
        for images, labels in loader:
            images = images.to(device)
            labels = labels.to(device)

            optimizer.zero_grad()
            outputs = model(images)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()

            loss_sum += loss.item() * images.size(0)
            predicted = outputs.argmax(dim=1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()

        acc = correct / max(total, 1) * 100.0
        avg_loss = loss_sum / max(total, 1)
        print(f"epoch={epoch + 1}/{args.epochs} loss={avg_loss:.4f} acc={acc:.2f}%", flush=True)

    output_pth = Path(args.output_pth)
    output_onnx = Path(args.output_onnx)
    output_pth.parent.mkdir(parents=True, exist_ok=True)
    tmp_pth = output_pth.with_suffix(output_pth.suffix + ".tmp")
    tmp_onnx = output_onnx.with_suffix(output_onnx.suffix + ".tmp")
    bak_pth = output_pth.with_suffix(output_pth.suffix + ".bak")
    bak_onnx = output_onnx.with_suffix(output_onnx.suffix + ".bak")

    torch.save(model.cpu().state_dict(), tmp_pth)
    export_onnx(model, tmp_onnx)

    if tmp_pth.stat().st_size <= 1024 or tmp_onnx.stat().st_size <= 1024:
        raise RuntimeError("Temporary model export is unexpectedly small.")

    if output_pth.exists():
        shutil.copy2(output_pth, bak_pth)
    if output_onnx.exists():
        shutil.copy2(output_onnx, bak_onnx)

    os.replace(tmp_pth, output_pth)
    os.replace(tmp_onnx, output_onnx)
    print(f"saved_pth={output_pth}", flush=True)
    print(f"saved_onnx={output_onnx}", flush=True)


if __name__ == "__main__":
    main()
