// Fill out your copyright notice in the Description page of Project Settings.


#include "VremDodgeProfile.h"

const FDodgeSequence& UVremDodgeProfile::GetDodgeSequence(EDodgeDirection Direction) const
{
    switch (Direction)
    {
    case EDodgeDirection::Forward:  
        return ForwardSequence;
    case EDodgeDirection::Backward: 
        return BackwardSequence;
    case EDodgeDirection::Left:     
        return LeftSequence;
    case EDodgeDirection::Right:    
        return RightSequence;
    default:                         
        return BackwardSequence;  // 입력 없을 시 후방 디폴트
    }
}

#if WITH_EDITOR
void UVremDodgeProfile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    auto ValidateSequence = [](FDodgeSequence& Seq)
        {
            // ForceDuration <= DodgeDuration
            Seq.ForceDuration = FMath::Min(Seq.ForceDuration, Seq.DodgeDuration);

            // i-frame은 DodgeDuration 안에 포함되어야 함
            Seq.IFrameStartTime = FMath::Min(Seq.IFrameStartTime, Seq.DodgeDuration);
            Seq.IFrameEndTime = FMath::Min(Seq.IFrameEndTime, Seq.DodgeDuration);

            // IFrameEndTime >= Seq.IFrameStartTime
            Seq.IFrameEndTime = FMath::Max(Seq.IFrameEndTime, Seq.IFrameStartTime);
        };

    ValidateSequence(ForwardSequence);
    ValidateSequence(BackwardSequence);
    ValidateSequence(LeftSequence);
    ValidateSequence(RightSequence);
}
#endif // WITH_EDITOR
