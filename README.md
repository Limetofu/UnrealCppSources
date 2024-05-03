/* --- Mesh Simplify 배치 ---*/
1. AActor (Actor 블루프린트 객체)
 - Actor Tag ("Simplify") 검사
   - Tag 순서 --> 꼭 지켜야 함!, Simplify 다음 태그를 참조하기 때문
   - Simplify
   - 111/222/123 (Color3f)
 - 있는 객체들만 모아 <AActor*, FVector> Pair의 Array로 생성
 
 - AActor* 복사해서 Simplify 한 후, 새로운 배열에 저장 (ClonedActors)
 -    1. Material 복사
      2. Material Parameter 변경
 - 새로운 ClonedActors를 월드에 SpawnActor
 - 그 후 Material의 ParameterValue를 Set ("SolidColor", Vector), ("IsLowPoly?", Scalar)

 - ClonedActorArray에 추가 (추후 사용할 수 있을지?)

2. AStaticMeshActor
 - AActor와 똑같음 => typename ActorType으로 템플릿 추가

3. AInstancedFoliageActor
 - Component 순회하면서, 각 UInstancedStaticMeshComponent Duplicate
 - Duplicate 과정 중, SetStaticMesh는 Simplify 후 집어넣기.
 - 원래 OriginalComponent들은 Visible false로 변환


/* --- Mesh Simplify ---*/
MeshSimplify <- DynamicMesh 변환 불가피
(1) UStaticMesh* -> DynamicMesh* 변환
(2) QEMSimplify 수행
(3) DynamicMesh* -> UStaticMesh* 재변환
