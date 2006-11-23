'*******************************************************************************************************
'  ProcessEvent.vb - Most basic data element in DatAWare
'  Copyright � 2006 - TVA, all rights reserved - Gbtc
'
'  Build Environment: VB.NET, Visual Studio 2005
'  Primary Developer: J. Ritchie Carroll, Operations Data Architecture [TVA]
'      Office: COO - TRNS/PWR ELEC SYS O, CHATTANOOGA, TN - MR 2W-C
'       Phone: 423/751-2827
'       Email: jrcarrol@tva.gov
'
'  Code Modification History:
'  -----------------------------------------------------------------------------------------------------
'  05/03/2006 - J. Ritchie Carroll
'       Initial version of source imported from 1.1 code library
'
'*******************************************************************************************************

' This is the most basic form of a point of data in DatAWare (used by ReadEvent and ReadRange)
Public Class DataPoint

#Region " Member Declaration "

    ' *******************************************************************************
    ' *                             Bit usage for *Flags*                           *
    ' *******************************************************************************
    ' * Bits    Mask    Description                                                 *
    ' * ----    ----    -----------                                                 *
    ' * 0-4     &H1F    Data quality indicator, a number between 0 and 31.          *
    ' *                 Maps to the same qualities as used by PMS process computer. *
    ' * 5-10    &H7E0   Index of time-zone used, number between 0 and 51            *
    ' * 11      &H800   DST indicator. When set, DST else Standard Time.            *
    ' *******************************************************************************

    Private m_tTag As TimeTag
    Private m_value As Single
    Private m_flags As Integer

    Private Const QualityMask As Integer = &H1F

#End Region

#Region " Public Code "

    Public Const BinaryLength As Integer = 16

#Region " Constructors "

    Public Sub New(ByVal binaryImage As Byte(), ByVal startIndex As Integer)

        MyBase.New()
        If binaryImage IsNot Nothing Then
            If binaryImage.Length - startIndex >= BinaryLength Then
                m_tTag = New TimeTag(BitConverter.ToDouble(binaryImage, startIndex))
                m_flags = BitConverter.ToInt32(binaryImage, startIndex + 8)
                m_value = BitConverter.ToSingle(binaryImage, startIndex + 12)
            Else
                Throw New ArgumentException("BinaryImage is too small.")
            End If
        Else
            Throw New ArgumentNullException("BinaryImage cannot be null.")
        End If

    End Sub

    Public Sub New(ByVal seconds As Double, ByVal value As Single, ByVal quality As Quality)

        MyClass.New(New TimeTag(seconds), value, quality)

    End Sub

    Public Sub New(ByVal timestamp As Date, ByVal value As Single, ByVal quality As Quality)

        MyClass.New(New TimeTag(timestamp), value, quality)

    End Sub

    Public Sub New(ByVal tTag As TimeTag, ByVal value As Single, ByVal quality As Quality)

        MyClass.New(tTag, value, -1)
        Me.Quality = quality

    End Sub

    Public Sub New(ByVal seconds As Double, ByVal value As Single, ByVal flags As Integer)

        MyClass.New(New TimeTag(seconds), value, flags)

    End Sub

    Public Sub New(ByVal timestamp As Date, ByVal value As Single, ByVal flags As Integer)

        MyClass.New(New TimeTag(timestamp), value, flags)

    End Sub

    Public Sub New(ByVal tTag As TimeTag, ByVal value As Single, ByVal flags As Integer)

        m_tTag = tTag
        m_value = value
        m_flags = flags

    End Sub

#End Region

    Public Property TTag() As TimeTag
        Get
            Return m_tTag
        End Get
        Set(ByVal value As TimeTag)
            m_tTag = value
        End Set
    End Property

    Public Property Value() As Single
        Get
            Return m_value
        End Get
        Set(ByVal value As Single)
            m_value = value
        End Set
    End Property

    Public Property Quality() As Quality
        Get
            Return CType((m_flags And QualityMask), Quality)
        End Get
        Set(ByVal value As Quality)
            m_flags = (m_flags Or value)
        End Set
    End Property

    Public ReadOnly Property BinaryImage() As Byte()
        Get
            Dim buffer As Byte() = CreateArray(Of Byte)(BinaryLength)

            ' Construct the binary IP buffer for this event
            Array.Copy(BitConverter.GetBytes(m_tTag.Value), 0, buffer, 0, 8)
            Array.Copy(BitConverter.GetBytes(m_flags), 0, buffer, 8, 4)
            Array.Copy(BitConverter.GetBytes(m_value), 0, buffer, 12, 4)

            Return buffer
        End Get
    End Property

#End Region

End Class
