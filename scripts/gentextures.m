
rangeX = -100:100;
rangeY = -100:100;
rangeT = -200:200;
[X,Y,Z] = meshgrid(rangeX,rangeY,rangeT);
Cx = randn(size(X));
Cy = randn(size(X));
F = 1.0 ./ (1.0 + X.^2 + Y.^2 + Z.^4);
CFx = Cx.*F;
CFx = ifftshift(CFx,1);
CFx = ifftshift(CFx,2);
CFx = ifftshift(CFx,3);
CFy = Cy.*F;
CFy = ifftshift(CFy,1);
CFy = ifftshift(CFy,2);
CFy = ifftshift(CFy,3);
Wx = ifftn(CFx);
Rx = real(Wx);
Ix = imag(Wx);
Ax = abs(Wx);
Wy = ifftn(CFy);
Ry = real(Wy);
Iy = imag(Wy);
Ay = abs(Wy);

Ax = Ax - min(Ax(:));
Ax = Ax / max(Ax(:));
Rx = Rx - min(Rx(:));
Rx = Rx / max(Rx(:));

Ay = Ay - min(Ay(:));
Ay = Ay / max(Ay(:));
Ry = Ry - min(Ry(:));
Ry = Ry / max(Ry(:));

clouds = true;
wind = true;
texdir = 'data/textures';
showImage = false;

if clouds
    disp('Write clouds');
    for k = 1:length(rangeT)
        if showImage
            imagesc(Rx(:,:,k).^2,[0,1]);
            colormap gray
            drawnow;
        end
        filename = strcat([texdir,'/clouds/cloud',num2str(k),'.png']);
        imwrite(repmat(Rx(:,:,k),1,1,3),filename,'Alpha',Rx(:,:,k));
    end
end
if wind
    disp('Write wind');
    for k = 1:length(rangeT)
        if showImage
            imagesc(Rx(:,:,k),[0,1]);
            colormap gray
            drawnow;
        end
        filename = strcat([texdir,'/windX/wind',num2str(k),'.png']);
        imwrite(repmat(Rx(:,:,k),1,1,3),filename,'Alpha',Rx(:,:,k));
    end
    for k = 1:length(rangeT)
        if showImage
            imagesc(Ry(:,:,k),[0,1]);
            colormap gray
            drawnow;
        end
        filename = strcat([texdir,'/windZ/wind',num2str(k),'.png']);
        imwrite(repmat(Ry(:,:,k),1,1,3),filename,'Alpha',Ry(:,:,k));
    end
end
