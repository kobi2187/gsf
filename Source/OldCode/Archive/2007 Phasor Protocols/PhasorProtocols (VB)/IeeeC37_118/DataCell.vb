'*******************************************************************************************************
'  DataCell.vb - IEEE C37.118 PMU Data Cell
'  Copyright � 2008 - TVA, all rights reserved - Gbtc
'
'  Build Environment: VB.NET, Visual Studio 2008
'  Primary Developer: J. Ritchie Carroll, Operations Data Architecture [TVA]
'      Office: COO - TRNS/PWR ELEC SYS O, CHATTANOOGA, TN - MR 2W-C
'       Phone: 423/751-2827
'       Email: jrcarrol@tva.gov
'
'  Code Modification History:
'  -----------------------------------------------------------------------------------------------------
'  11/12/2004 - J. Ritchie Carroll
'       Initial version of source generated
'
'*******************************************************************************************************

Imports System.Runtime.Serialization
Imports PhasorProtocols.IeeeC37_118.Common

Namespace IeeeC37_118

    ' This data cell represents what most might call a "field" in table of rows - it is a single unit of data for a specific PMU
    <CLSCompliant(False), Serializable()> _
    Public Class DataCell

        Inherits DataCellBase

        Protected Sub New()
        End Sub

        Protected Sub New(ByVal info As SerializationInfo, ByVal context As StreamingContext)

            MyBase.New(info, context)

        End Sub

        Public Sub New(ByVal parent As IDataFrame, ByVal configurationCell As IConfigurationCell)

            MyBase.New(parent, False, configurationCell, MaximumPhasorValues, MaximumAnalogValues, MaximumDigitalValues)

            Dim x As Int32

            ' Initialize phasor values and frequency value with an empty value
            For x = 0 To configurationCell.PhasorDefinitions.Count - 1
                PhasorValues.Add(New PhasorValue(Me, configurationCell.PhasorDefinitions(x), Single.NaN, Single.NaN))
            Next

            ' Initialize frequency and df/dt
            FrequencyValue = New FrequencyValue(Me, configurationCell.FrequencyDefinition, Single.NaN, Single.NaN)

            ' Initialize analog values
            For x = 0 To configurationCell.AnalogDefinitions.Count - 1
                AnalogValues.Add(New AnalogValue(Me, configurationCell.AnalogDefinitions(x), Single.NaN))
            Next

            ' Initialize any digital values
            For x = 0 To configurationCell.DigitalDefinitions.Count - 1
                DigitalValues.Add(New DigitalValue(Me, configurationCell.DigitalDefinitions(x), -1))
            Next

        End Sub

        Public Sub New(ByVal dataCell As IDataCell)

            MyBase.New(dataCell)

        End Sub

        Public Sub New(ByVal parent As IDataFrame, ByVal state As DataFrameParsingState, ByVal index As Int32, ByVal binaryImage As Byte(), ByVal startIndex As Int32)

            MyBase.New(parent, False, MaximumPhasorValues, MaximumAnalogValues, MaximumDigitalValues, _
                New DataCellParsingState(state.ConfigurationFrame.Cells(index), _
                    AddressOf IeeeC37_118.PhasorValue.CreateNewPhasorValue, _
                    AddressOf IeeeC37_118.FrequencyValue.CreateNewFrequencyValue, _
                    AddressOf IeeeC37_118.AnalogValue.CreateNewAnalogValue, _
                    AddressOf IeeeC37_118.DigitalValue.CreateNewDigitalValue), _
                binaryImage, startIndex)

        End Sub

        Friend Shared Function CreateNewDataCell(ByVal parent As IChannelFrame, ByVal state As IChannelFrameParsingState(Of IDataCell), ByVal index As Int32, ByVal binaryImage As Byte(), ByVal startIndex As Int32) As IDataCell

            Return New DataCell(parent, state, index, binaryImage, startIndex)

        End Function

        Public Overrides ReadOnly Property DerivedType() As System.Type
            Get
                Return Me.GetType()
            End Get
        End Property

        Public Shadows ReadOnly Property Parent() As DataFrame
            Get
                Return MyBase.Parent
            End Get
        End Property

        Public Shadows Property ConfigurationCell() As ConfigurationCell
            Get
                Return MyBase.ConfigurationCell
            End Get
            Set(ByVal value As ConfigurationCell)
                MyBase.ConfigurationCell = value
            End Set
        End Property

        Public Shadows Property StatusFlags() As StatusFlags
            Get
                Return MyBase.StatusFlags And Not (StatusFlags.UnlockedTimeMask Or StatusFlags.TriggerReasonMask)
            End Get
            Set(ByVal value As StatusFlags)
                MyBase.StatusFlags = (MyBase.StatusFlags And (StatusFlags.UnlockedTimeMask Or StatusFlags.TriggerReasonMask)) Or value
            End Set
        End Property

        Public Property UnlockedTime() As UnlockedTime
            Get
                Return MyBase.StatusFlags And StatusFlags.UnlockedTimeMask
            End Get
            Set(ByVal value As UnlockedTime)
                MyBase.StatusFlags = (MyBase.StatusFlags And Not StatusFlags.UnlockedTimeMask) Or value
                SynchronizationIsValid = (value = IeeeC37_118.UnlockedTime.SyncLocked)
            End Set
        End Property

        Public Property TriggerReason() As TriggerReason
            Get
                Return MyBase.StatusFlags And StatusFlags.TriggerReasonMask
            End Get
            Set(ByVal value As TriggerReason)
                MyBase.StatusFlags = (MyBase.StatusFlags And Not StatusFlags.TriggerReasonMask) Or value
                PmuTriggerDetected = (value <> IeeeC37_118.TriggerReason.Manual)
            End Set
        End Property

        Public Overrides Property DataIsValid() As Boolean
            Get
                Return (StatusFlags And IeeeC37_118.StatusFlags.DataIsValid) = 0
            End Get
            Set(ByVal value As Boolean)
                If value Then
                    StatusFlags = StatusFlags And Not IeeeC37_118.StatusFlags.DataIsValid
                Else
                    StatusFlags = StatusFlags Or IeeeC37_118.StatusFlags.DataIsValid
                End If
            End Set
        End Property

        Public Overrides Property SynchronizationIsValid() As Boolean
            Get
                Return (StatusFlags And IeeeC37_118.StatusFlags.PmuSynchronizationError) = 0
            End Get
            Set(ByVal value As Boolean)
                If value Then
                    StatusFlags = StatusFlags And Not IeeeC37_118.StatusFlags.PmuSynchronizationError
                Else
                    StatusFlags = StatusFlags Or IeeeC37_118.StatusFlags.PmuSynchronizationError
                End If
            End Set
        End Property

        Public Overrides Property PmuError() As Boolean
            Get
                Return (StatusFlags And IeeeC37_118.StatusFlags.PmuError) > 0
            End Get
            Set(ByVal value As Boolean)
                If value Then
                    StatusFlags = StatusFlags Or IeeeC37_118.StatusFlags.PmuError
                Else
                    StatusFlags = StatusFlags And Not IeeeC37_118.StatusFlags.PmuError
                End If
            End Set
        End Property

        Public Overrides Property DataSortingType() As DataSortingType
            Get
                Return IIf((StatusFlags And IeeeC37_118.StatusFlags.DataSortingType) = 0, PhasorProtocols.DataSortingType.ByTimestamp, PhasorProtocols.DataSortingType.ByArrival)
            End Get
            Set(ByVal value As DataSortingType)
                If value = PhasorProtocols.DataSortingType.ByTimestamp Then
                    StatusFlags = StatusFlags And Not IeeeC37_118.StatusFlags.DataSortingType
                Else
                    StatusFlags = StatusFlags Or IeeeC37_118.StatusFlags.DataSortingType
                End If
            End Set
        End Property

        Public Property PmuTriggerDetected() As Boolean
            Get
                Return (StatusFlags And IeeeC37_118.StatusFlags.PmuTriggerDetected) > 0
            End Get
            Set(ByVal value As Boolean)
                If value Then
                    StatusFlags = StatusFlags Or IeeeC37_118.StatusFlags.PmuTriggerDetected
                Else
                    StatusFlags = StatusFlags And Not IeeeC37_118.StatusFlags.PmuTriggerDetected
                End If
            End Set
        End Property

        Public Property ConfigurationChangeDetected() As Boolean
            Get
                Return (StatusFlags And IeeeC37_118.StatusFlags.ConfigurationChanged) > 0
            End Get
            Set(ByVal value As Boolean)
                If value Then
                    StatusFlags = StatusFlags Or IeeeC37_118.StatusFlags.ConfigurationChanged
                Else
                    StatusFlags = StatusFlags And Not IeeeC37_118.StatusFlags.ConfigurationChanged
                End If
            End Set
        End Property

        Public Overrides ReadOnly Property Attributes() As Dictionary(Of String, String)
            Get
                Dim baseAttributes As Dictionary(Of String, String) = MyBase.Attributes

                baseAttributes.Add("Unlocked Time", UnlockedTime & ": " & [Enum].GetName(GetType(UnlockedTime), UnlockedTime))
                baseAttributes.Add("PMU Trigger Detected", PmuTriggerDetected)
                baseAttributes.Add("Trigger Reason", TriggerReason & ": " & [Enum].GetName(GetType(TriggerReason), TriggerReason))
                baseAttributes.Add("Configuration Change Detected", ConfigurationChangeDetected)

                Return baseAttributes
            End Get
        End Property

    End Class

End Namespace